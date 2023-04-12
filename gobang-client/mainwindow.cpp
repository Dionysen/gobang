#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "game.h"
#include "lobby.h"
#include "onlinegame.h"

#include "tcpclient.h"
#include <qmessagebox.h>
#include <qobjectdefs.h>

// #include "threadpool.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    // pool.init();
    this->setFixedSize(1000, 750);

    ui->stackedWidget->addWidget(Home);
    ui->stackedWidget->addWidget(Lobby);
    ui->stackedWidget->addWidget(Game);
    ui->stackedWidget->addWidget(OnlineGame);

    // Mainwindow connection
    connect(ui->actionNew_Game, SIGNAL(triggered(bool)), this,
            SLOT(toGamePage()));
    connect(ui->actionBack, SIGNAL(triggered(bool)), this, SLOT(backToHome()));
    connect(ui->actionExit, SIGNAL(triggered(bool)), this, SLOT(exit()));
    connect(ui->actionRetract, &QAction::triggered, Game, &game::retract);
    connect(ui->actionRobot_Setting, &QAction::triggered, Game, &game::setting);

    // Home connection
    connect(Home, SIGNAL(signalNewGame()), this, SLOT(toGamePage()));
    // connect(Home, SIGNAL(signalHelp()), this, SLOT(help()));
    connect(Home, SIGNAL(signalExit()), this, SLOT(exit()));

    // Lobby connection
    connect(Lobby, &lobby::signalBackToHomeNoAsk, this, [=] {
        recvThread->terminate();
        emit backToHomeNoAsk();
    });
    connect(Lobby, &lobby::signalWatchMatch, this, [=] {
        if (Lobby->getRoomID() != -1) {
            OnlineGame->newGame();
            OnlineGame->status = WATCHING;
            tcpClient.watchMatch(Lobby->getRoomID());
            OnlineGame->update();
        } else {
            QMessageBox::information(this, "WATCH",
                                     "Please select a room to watch!",
                                     QMessageBox::Ok, QMessageBox::Ok);
        }
    });
    connect(Lobby, SIGNAL(signalCreateRoom()), this, SLOT(createRoom()));
    connect(Lobby, SIGNAL(siganlJoinRoom()), this, SLOT(joinRoom()));

    // Game

    connect(Game, &game::signalBackToHome, this, &MainWindow::backToHome);
    connect(Game, &game::signalBackNoAsk, this,
            [=] { // back to home directly
                ui->stackedWidget->setCurrentWidget(Home);
                Game->newGame();
            });

    // Online Game
    connect(OnlineGame, &onlinegame::signalPrepare, this,
            [=] { tcpClient.prepared(); });

    connect(OnlineGame, &onlinegame::signalDrop, this, [=] {
        tcpClient.drop(OnlineGame->getCurrentChess().x(),
                       OnlineGame->getCurrentChess().y());
    });

    connect(OnlineGame, &onlinegame::signalBack, this,
            [=] { // back to lobby
                if (ui->stackedWidget->currentWidget() == Lobby) {
                } else {
                    ui->stackedWidget->setCurrentWidget(Lobby);
                    tcpClient.quitRoom();
                }
            });

    connect(OnlineGame, &onlinegame::signalBackToLobby, this,
            [=] { ui->stackedWidget->setCurrentWidget(Lobby); });

    connect(OnlineGame, &onlinegame::signalConcede, this,
            [=] { tcpClient.concede(); });

    connect(OnlineGame, &onlinegame::signalRequestRetract, this, [=] {
        if (OnlineGame->selfTurn && !OnlineGame->isEmptyBoard()) {
            QMessageBox::information(
                this, "RETRACT",
                "Error!\nYou can retract only in the rival turn!");
        } else if (OnlineGame->isEmptyBoard()) {
            QMessageBox::information(this, "RETRACT",
                                     "Error!\nChess board is empty!");
        } else if (!OnlineGame->isEmptyBoard() && !OnlineGame->selfTurn) {
            tcpClient.requestRetract();
        }
    });

    connect(OnlineGame, &onlinegame::signalRespondRetract, this,
            [=](bool anwser) { tcpClient.repondRetract(anwser); });

    connect(OnlineGame, &onlinegame::signalRequestReplay, this,
            [=] { tcpClient.requestReplay(); });

    connect(OnlineGame, &onlinegame::signalRespondReplay, this,
            [=](bool anwser) { tcpClient.repondReplay(anwser); });

    connect(OnlineGame, &onlinegame::signalRestartGame, this,
            [=] { tcpClient.restartGame(); });

    connect(OnlineGame, &onlinegame::siganlUpdateUserInfo, this, [=] {
        if (OnlineGame->status == IN_LOBBY) {
            OnlineGame->setWatcher(false);
            ui->stackedWidget->setCurrentWidget(Lobby);
        } else if (OnlineGame->status == IN_ROOM) {
            OnlineGame->setWatcher(false);
            ui->stackedWidget->setCurrentWidget(OnlineGame);
            OnlineGame->updateRoomInfo();
        } else if (OnlineGame->status == GAMING) {
            OnlineGame->setWatcher(false);
            ui->stackedWidget->setCurrentWidget(OnlineGame);
        } else if (OnlineGame->status == WATCHING) {
            ui->stackedWidget->setCurrentWidget(OnlineGame);
            OnlineGame->setWatcher(true);
        }
    });

    // child thread, it can also use thread pool processing

    connect(Home, &home::signalConnectToServer, this, [=] {
        if (!tcpClient.joinLobby()) {
            QMessageBox::critical(this, "Connect error!",
                                  "Failed to connect to server, please check "
                                  "your internet and try it again.");
        } else {
            ui->stackedWidget->setCurrentWidget(Lobby);
            Lobby->updateTable();
            recvThread->start();
            emit sendConnfd(tcpClient.getSockFD());
        }

        // std::thread recvInfoThread(&MainWindow::parseThreadHandle,
        //                            std::ref(*OnlineGame),
        //                            std::ref(*Lobby),
        //                            tcpClient.getSockFD(),
        //                            std::ref(tcpClient), std::ref(pool));
        // std::thread recvInfoThread(handle, 1, 12);
        // recvInfoThread.join();
        // recvInfoThread.detach();
    });

    connect(this, &MainWindow::sendConnfd, recvThread, &recvthread::recvSockfd);

    connect(recvThread, &recvthread::sendBuff, this, [=](std::string buff) {
        tcpClient.parseInformation(*OnlineGame, *Lobby, buff);
    });
}

// void MainWindow::parseThreadHandle(onlinegame &OnlineGame, lobby &Lobby,
//                                    int connfd, tcpclient &tcpClient,
//                                    threadpool &pool) {
//     while (true) {
//         char buff[1024];
//         memset(buff, 0, sizeof(buff));
//         int len{0};
//         recv(connfd, &len, sizeof(int), 0);
//         std::cout << "len = " << len << std::endl;

//         ssize_t isize = recv(connfd, buff, len, 0);
//         if (isize <= 0) {
//             break;
//         }
//
//         pool.submit(tcpClient.parseInformation, std::ref(OnlineGame),
//                     std::ref(Lobby), std::string(buff));
//     }
// }

MainWindow::~MainWindow() { delete ui; }

void MainWindow::closeEvent(QCloseEvent *event) {
    if (ui->stackedWidget->currentWidget() == Lobby ||
        ui->stackedWidget->currentWidget() == OnlineGame) {

        QMessageBox::StandardButton result = QMessageBox::question(
            this, "EXIT",
            "Exiting now will disconnect the server and will "
            "not preserve the current match!",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

        if (result == QMessageBox::Yes) {
            recvThread->terminate();
            tcpClient.quitLobby();
            event->accept();
        } else {
            event->ignore();
        }
    }
}

void MainWindow::toGamePage() {
    ui->stackedWidget->setCurrentWidget(Game);
    Game->newGame();
};

void MainWindow::backToHome() {
    if (Game->isGaming()) {
        switch (QMessageBox::question(
            this, "BACK",
            "Are you sure you want to back? Game status will not be saved.",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No)) {
        case QMessageBox::Yes:
            Game->setGameStatus(false);                // Set not game
            ui->stackedWidget->setCurrentWidget(Home); // back to home
            break;
        case QMessageBox::No:
            break;
        default:
            break;
        }
    }
}
void MainWindow::backToHomeNoAsk() {
    ui->stackedWidget->setCurrentWidget(Home);
    Game->newGame();
    tcpClient.quitLobby();
}

void MainWindow::exit() {
    switch (QMessageBox::question(
        this, "EXIT", "Are you sure you want to quit game?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No)) {
    case QMessageBox::Yes:
        return ::exit(0);
        break;
    case QMessageBox::No:
        break;
    default:
        break;
    }
}

void MainWindow::createRoom() // Send the information of creating a room,
                              // receive the lobby information, update the
                              // display, set the current widget to game, and
                              // set yourself to play black chess
{
    OnlineGame->newGame();
    tcpClient.createRoom();
    Lobby->updateTable();
    ui->stackedWidget->setCurrentWidget(OnlineGame);
    OnlineGame->setSelfColor(BLACK_CHESS);
    OnlineGame->updateRoomInfo();
}

void MainWindow::joinRoom() // Send join room information, receive lobby
                            // information, update display, set the current
                            // widget to game, and set yourself to play white
                            // chess
{
    int num = Lobby->getRoomNumOfPlayers();
    if (-1 != Lobby->getRoomID()) {
        if (num == 1) {
            OnlineGame->newGame();
            tcpClient.joinRoom(Lobby->getRoomID());
            Lobby->updateTable();
            ui->stackedWidget->setCurrentWidget(OnlineGame);
            OnlineGame->setSelfColor(WHITE_CHESS);
            OnlineGame->updateRoomInfo();
        } else {
            QMessageBox::critical(
                this, "JOIN",
                "Room is already full!\nPlease select another room!");
        }
    } else {
        QMessageBox::critical(this, "JOIN",
                              "Invaild room!\nPlease select a room!");
    }
}

void MainWindow::dropChess() // drop
{
    tcpClient.drop(
        OnlineGame->getCurrentChess().x(),
        OnlineGame->getCurrentChess()
            .y()); // Use getpoint in the game to get the position of the
                   // current piece and pass it to tcpclient to play chess
}
