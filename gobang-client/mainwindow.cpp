#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "game.h"
#include "lobby.h"
#include "onlinegame.h"
#include "recvthread.h"
#include "tcpclient.h"
#include "threadpool.h"
#include <cstddef>
#include <functional>
#include <iostream>
#include <ostream>
#include <qaction.h>
#include <qevent.h>
#include <qobjectdefs.h>
#include <qpolygon.h>
#include <qpushbutton.h>
#include <sched.h>
#include <string>
#include <thread>
#include <unistd.h>

void handle(int a, int b) {}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    pool.init();
    this->setFixedSize(1000, 750);

    ui->stackedWidget->addWidget(Home);
    ui->stackedWidget->addWidget(Lobby);
    ui->stackedWidget->addWidget(Game);
    ui->stackedWidget->addWidget(OnlineGame);

    // 主窗口的信号连接
    connect(ui->actionNew_Game, SIGNAL(triggered(bool)), this,
            SLOT(toGamePage()));
    connect(ui->actionBack, SIGNAL(triggered(bool)), this, SLOT(backToHome()));
    connect(ui->actionExit, SIGNAL(triggered(bool)), this, SLOT(exit()));
    connect(ui->actionRetract, &QAction::triggered, Game, &game::retract);
    connect(ui->actionRobot_Setting, &QAction::triggered, Game, &game::setting);

    // 主菜单的信号连接
    connect(Home, SIGNAL(signalNewGame()), this, SLOT(toGamePage()));
    // connect(Home, SIGNAL(signalHelp()), this, SLOT(help()));
    connect(Home, SIGNAL(signalExit()), this, SLOT(exit()));

    // 游戏大厅的信号连接
    connect(Lobby, SIGNAL(signalBackToHomeNoAsk()), this,
            SLOT(backToHomeNoAsk()));
    connect(Lobby, &lobby::signalWatchMatch, this, [=] { // 观战
        if (Lobby->getRoomID() != -1) {
            OnlineGame->newGame();
            OnlineGame->status = WATCHING;
            tcpClient.watchMatch(Lobby->getRoomID());
            OnlineGame->update();
        } else {
            QMessageBox mb;
            mb.setWindowTitle(tr("WATCH"));
            mb.setText(tr("Error!"));
            mb.setInformativeText(tr("Please select a room to watch!"));
            mb.setStandardButtons(QMessageBox::Ok | QMessageBox::Default);
            mb.setDefaultButton(QMessageBox::Yes);
            switch (mb.exec()) {
            case QMessageBox::Yes:
                break;
            default:
                break;
            }
        }
    });
    connect(Lobby, SIGNAL(signalCreateRoom()), this, SLOT(createRoom()));
    connect(Lobby, SIGNAL(siganlJoinRoom()), this, SLOT(joinRoom()));

    // 游戏信号

    connect(Game, &game::signalBackToHome, this, &MainWindow::backToHome);
    connect(Game, &game::signalBackNoAsk, this, [=] { // 直接回到首页
        ui->stackedWidget->setCurrentWidget(Home);
        Game->newGame();
    });

    // 对战游戏
    connect(OnlineGame, &onlinegame::signalPrepare, this,
            [=] { tcpClient.prepared(); });

    connect(OnlineGame, &onlinegame::signalDrop, this, [=] {
        tcpClient.drop(OnlineGame->getCurrentChess().x(),
                       OnlineGame->getCurrentChess().y());
    });

    connect(OnlineGame, &onlinegame::signalBack, this, [=] { // 回到大厅
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
            QMessageBox mb;
            mb.setWindowTitle(tr("RETRACT"));
            mb.setText(tr("Error!"));
            mb.setInformativeText(
                tr("You can retract only in the rival turn!"));
            mb.setStandardButtons(QMessageBox::Ok | QMessageBox::Default);
            mb.setDefaultButton(QMessageBox::Yes);
            switch (mb.exec()) {
            case QMessageBox::Yes:
                break;
            default:
                break;
            }
        } else if (OnlineGame->isEmptyBoard()) {
            QMessageBox mb;
            mb.setWindowTitle(tr("RETRACT"));
            mb.setText(tr("Error!"));
            mb.setInformativeText(tr("Chess board is empty!"));
            mb.setStandardButtons(QMessageBox::Ok | QMessageBox::Default);
            mb.setDefaultButton(QMessageBox::Yes);
            switch (mb.exec()) {
            case QMessageBox::Yes:
                break;
            default:
                break;
            }
        } else if (!OnlineGame->isEmptyBoard() && !OnlineGame->selfTurn) {
            tcpClient.requestRetract();
        }
    });

    connect(OnlineGame, &onlinegame::signalRespondRetract, this,
            [=](bool anwser) { tcpClient.repondRetract(anwser); });

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

    // 子线程，用以处理消息，也可以使用线程池处理消息
    recvthread *recvThread = new recvthread(this);

    connect(Home, &home::signalConnectToServer, this, [=] {
        if (!tcpClient.joinLobby()) {
            QMessageBox mb;
            mb.setWindowTitle(tr("ERROR"));
            mb.setText(tr("Connect error!"));
            mb.setInformativeText(
                tr("Failed to connect to server, please check "
                   "your internet and try it again."));
            mb.setStandardButtons(QMessageBox::Yes | QMessageBox::Default);
            mb.setDefaultButton(QMessageBox::Yes);
            switch (mb.exec()) {
            case QMessageBox::Yes:
                break;
            default:
                break;
            }
        } else {
            ui->stackedWidget->setCurrentWidget(Lobby);
            Lobby->updateTable();
            recvThread->start();
            emit sendConnfd(tcpClient.getSockFD());
        }

        // std::thread recvInfoThread(&MainWindow::parseThreadHandle,
        //                            std::ref(*OnlineGame), std::ref(*Lobby),
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
//         std::cout << "子线程收到消息：" << buff << std::endl;
//         pool.submit(tcpClient.parseInformation, std::ref(OnlineGame),
//                     std::ref(Lobby), std::string(buff));
//     }
// }

MainWindow::~MainWindow() { delete ui; }

void MainWindow::toGamePage() {
    ui->stackedWidget->setCurrentWidget(Game);
    Game->newGame();
};

void MainWindow::backToHome() {
    if (Game->isGaming()) {
        QMessageBox mb;
        mb.setWindowTitle(tr("BACK"));
        mb.setText(tr("Back to home!"));
        mb.setInformativeText(tr("Are you sure you want to back? "
                                 "Game status will not be saved."));
        mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No |
                              QMessageBox::Default);
        mb.setDefaultButton(QMessageBox::Yes);
        switch (mb.exec()) {
        case QMessageBox::Yes:
            Game->setGameStatus(false);                // 游戏状态设置为0
            ui->stackedWidget->setCurrentWidget(Home); // 显示主界面
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
    QMessageBox mb;
    mb.setWindowTitle(tr("EXIT"));
    mb.setText(tr("Exit!"));
    mb.setInformativeText(tr("Are you sure you want to quit game?"));
    mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No |
                          QMessageBox::Default);
    mb.setDefaultButton(QMessageBox::Yes);
    switch (mb.exec()) {
    case QMessageBox::Yes:
        return ::exit(0);
        break;
    case QMessageBox::No:
        break;
    default:
        break;
    }
}

void MainWindow::
    createRoom() // 发送创建房间的信息，接收大厅信息，更新显示，设置当前widget为game，设置自身执黑棋
{
    OnlineGame->newGame();
    tcpClient.createRoom();
    Lobby->updateTable();
    ui->stackedWidget->setCurrentWidget(OnlineGame);
    OnlineGame->setSelfColor(BLACK_CHESS);
    OnlineGame->updateRoomInfo();
}

void MainWindow::
    joinRoom() // 发送加入房间信息，接收大厅信息，更新显示，设置当前widget为game，设置自身执白棋
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
            QMessageBox mb;
            mb.setWindowTitle(tr("JOIN"));
            mb.setText(tr("Room is already full!"));
            mb.setInformativeText(tr("Please select another room!"));
            mb.setStandardButtons(QMessageBox::Yes | QMessageBox::Default);
            mb.setDefaultButton(QMessageBox::Yes);
            switch (mb.exec()) {
            case QMessageBox::Yes:
                break;
            default:
                break;
            }
        }
    } else {
        QMessageBox mb;
        mb.setWindowTitle(tr("JOIN"));
        mb.setText(tr("Invaild room!"));
        mb.setInformativeText(tr("Please select a room!"));
        mb.setStandardButtons(QMessageBox::Yes | QMessageBox::Default);
        mb.setDefaultButton(QMessageBox::Yes);
        switch (mb.exec()) {
        case QMessageBox::Yes:
            break;
        default:
            break;
        }
    }
}

void MainWindow::dropChess() // 下棋
{
    tcpClient.drop(
        OnlineGame->getCurrentChess().x(),
        OnlineGame->getCurrentChess()
            .y()); // 使用Game中的getpoint获取当前棋子的位置，传递给tcpclient下棋
}
