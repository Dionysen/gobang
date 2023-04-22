#include "onlinegame.h"
#include "chess.h"
#include "ui_onlinegame.h"
#include <iostream>
#include <qmessagebox.h>
#include <qnamespace.h>
#include <string>

onlinegame::onlinegame(QWidget *parent)
    : QWidget(parent), ui(new Ui::onlinegame) {
    ui->setupUi(this);
    this->setMouseTracking(true);
    m_blackPlayerName = "Black Player";
    m_whitePlayerName = "White Player";
    newGame();

    ui->whiteTurnPtn->hide();
    ui->whiteTurnPtn->hide();

    connect(ui->retractBtn, &QPushButton::clicked, this,
            [=] { emit signalRequestRetract(); });

    connect(ui->concedeBtn, &QPushButton::clicked, this,
            [=] { emit signalConcede(); });

    connect(ui->replayBtn, &QPushButton::clicked, this, [=] {
        emit signalRequestReplay();
        std::cout << "request replay!\n";
    });

    connect(ui->backBtn, &QPushButton::clicked, this,
            [=] { emit signalBack(); });

    connect(ui->preBtn, &QPushButton::clicked, this, [=] {
        emit signalPrepare(); // send signal preparing
    });

    // timer
    connect(timer, &QTimer::timeout, this, [=] {
        if (status == GAMING) {
            if (selfColor == BLACK_CHESS) {
                if (selfTurn)
                    m_blackTime--;
                else
                    m_whiteTime--;
            } else if (selfColor == WHITE_CHESS) {
                if (selfTurn)
                    m_whiteTime--;
                else
                    m_blackTime--;
            }
        }
        updateRoomInfo();
    });
    timer->start(1000);
}

onlinegame::~onlinegame() { delete ui; }

void onlinegame::newGame() {
    status = IN_LOBBY;
    selfTurn = false;
    selfColor = NO_CHESS;
    step = 0;
    m_blackPlayerId = 0;
    m_whitePlayerId = 1;
    m_blackTime = 1200;
    m_whiteTime = 1200;
    clearBoard();
    currentChess = chess(-1, -1, NO_CHESS, -1);
    if (selfColor == BLACK_CHESS)
        m_blackPlayerName = m_name;
    else if (selfColor == WHITE_CHESS)
        m_whitePlayerName = m_name;
    update();
}

void onlinegame::setWatcher(bool isWatcher) {
    if (isWatcher) {
        ui->retractBtn->setEnabled(false);
        ui->replayBtn->setEnabled(false);
        ui->concedeBtn->setEnabled(false);
    } else {
        ui->retractBtn->setEnabled(true);
        ui->replayBtn->setEnabled(true);
        ui->concedeBtn->setEnabled(true);
    }
}

void onlinegame::updateRoomInfo() { // update room info
    // hide white player info when it is empty
    if (m_whitePlayerName == "None") {
        ui->white->hide();
        ui->whiteName->hide();
        ui->whiteID->hide();
        ui->whiteTime->hide();
    } else {
        ui->white->show();
        ui->whiteName->show();
        ui->whiteID->show();
        ui->whiteTime->show();
    }

    // black
    ui->blackID->setText(QString::fromStdString(
        "Black ID: " + std::to_string(this->m_blackPlayerId)));
    ui->blackName->setText(
        QString::fromStdString("Black Name: " + m_blackPlayerName));
    ui->blackTime->setText(
        QString::fromStdString("Time: " + std::to_string(m_blackTime / 60) +
                               ":" + std::to_string(m_blackTime % 60)));
    // white
    ui->whiteID->setText(QString::fromStdString(
        "White ID: " + std::to_string(this->m_whitePlayerId)));
    ui->whiteName->setText(
        QString::fromStdString("White Name: " + m_whitePlayerName));
    ui->whiteTime->setText(
        QString::fromStdString("Time: " + std::to_string(m_whiteTime / 60) +
                               ":" + std::to_string(m_whiteTime % 60)));

    if (status == GAMING ) { // GAMING
        ui->preBtn->setFixedSize(0, 0);
    }
    else if (status == WATCHING) {
        ui->preBtn->setFixedSize(0, 0);
    }
    else {
        ui->preBtn->setFixedSize(240, 100);
    }
        

    if (status == GAMING) { // highlight
        if (getTurn() == BLACK_CHESS) {
            ui->blackTurnPtn->show();
            ui->whiteTurnPtn->hide();
        } else {
            ui->blackTurnPtn->hide();
            ui->whiteTurnPtn->show();
        }
    } else {
        ui->whiteTurnPtn->hide();
        ui->whiteTurnPtn->hide();
    }
}

// event function
void onlinegame::paintEvent(QPaintEvent *event) {
    QPainter paint(this);

    QColor boardBackgroundColor(206, 178, 152);
    paint.setPen(boardBackgroundColor);
    paint.setBrush(boardBackgroundColor);
    // paint.drawRect(POS - WIDTH / 2, POS - WIDTH / 2, 15 * WIDTH, 15 * WIDTH);

    paint.setPen(QPen(QColor(Qt::black), 1));
    for (int i = 0; i < ROW_COLUM; i++) {
        paint.drawLine(POS + i * WIDTH, POS, POS + i * WIDTH, POS + 14 * WIDTH);
    }
    for (int i = 0; i < ROW_COLUM; i++) {
        paint.drawLine(POS, POS + i * WIDTH, POS + 14 * WIDTH, POS + i * WIDTH);
    }

    paint.setPen(QPen(QColor(Qt::black), 2));
    paint.drawLine(POS, POS, POS + 14 * WIDTH, POS);
    paint.drawLine(POS, POS, POS, POS + 14 * WIDTH);
    paint.drawLine(POS + 14 * WIDTH, POS, POS + 14 * WIDTH, POS + 14 * WIDTH);
    paint.drawLine(POS, POS + 14 * WIDTH, POS + 14 * WIDTH, POS + 14 * WIDTH);

    paint.setBrush(Qt::black);
    paint.drawEllipse(POS + 3 * WIDTH - POS_SIZE, POS + 3 * WIDTH - POS_SIZE,
                      POS_SIZE * 2, POS_SIZE * 2);
    paint.drawEllipse(POS + 11 * WIDTH - POS_SIZE, POS + 3 * WIDTH - POS_SIZE,
                      POS_SIZE * 2, POS_SIZE * 2);
    paint.drawEllipse(POS + 3 * WIDTH - POS_SIZE, POS + 11 * WIDTH - POS_SIZE,
                      POS_SIZE * 2, POS_SIZE * 2);
    paint.drawEllipse(POS + 11 * WIDTH - POS_SIZE, POS + 11 * WIDTH - POS_SIZE,
                      POS_SIZE * 2, POS_SIZE * 2);
    paint.drawEllipse(POS + 7 * WIDTH - POS_SIZE, POS + 7 * WIDTH - POS_SIZE,
                      POS_SIZE * 2, POS_SIZE * 2);

    for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < 15; ++j) {
            if (positionStatus[i][j] == BLACK_CHESS) {
                paint.setPen(QPen(QColor(Qt::black), 1));
                paint.setBrush(Qt::black);
                paint.drawEllipse(POS + i * WIDTH - CHESS_RATIO,
                                  POS + j * WIDTH - CHESS_RATIO,
                                  CHESS_RATIO * 2, CHESS_RATIO * 2);
            } else if (positionStatus[i][j] == WHITE_CHESS) {
                paint.setPen(QPen(QColor(Qt::black), 1));
                paint.setBrush(Qt::white);
                paint.drawEllipse(POS + i * WIDTH - CHESS_RATIO,
                                  POS + j * WIDTH - CHESS_RATIO,
                                  CHESS_RATIO * 2, CHESS_RATIO * 2);
            }
        }
    }
    // paint current chess
    if (currentChess.x() != -1 && currentChess.y() != -1) {
        paint.setPen(Qt::green);
        paint.setBrush(Qt::green);
        QPolygon triangle;
        triangle.setPoints(3, POS + currentChess.x() * WIDTH,
                           POS + currentChess.y() * WIDTH,
                           POS + currentChess.x() * WIDTH,
                           POS + currentChess.y() * WIDTH + CHESS_RATIO,
                           POS + currentChess.x() * WIDTH + CHESS_RATIO,
                           POS + currentChess.y() * WIDTH);
        paint.drawPolygon(triangle);
    }

    if (hoverPosition.x() >= 0 && hoverPosition.x() < 15 &&
        hoverPosition.y() >= 0 && hoverPosition.y() < 15 && status == GAMING) {
        bool isExist = false;
        for (auto i = m_board.begin(); i != m_board.end(); i++) {
            if (i->x() == hoverPosition.x() && i->y() == hoverPosition.y())
                isExist = true;
        }
        if (!isExist) {
            if (selfColor == WHITE_CHESS) {
                QColor color = Qt::white;
                color.setAlphaF(0.5);
                paint.setPen(Qt::black);
                paint.setBrush(color);
                paint.drawEllipse(POS + hoverPosition.x() * WIDTH - CHESS_RATIO,
                                  POS + hoverPosition.y() * WIDTH - CHESS_RATIO,
                                  CHESS_RATIO * 2, CHESS_RATIO * 2);
            } else {
                QColor color = Qt::black;
                color.setAlphaF(0.5);
                paint.setPen(QPen(color));
                paint.setBrush(color);
                paint.drawEllipse(POS + hoverPosition.x() * WIDTH - CHESS_RATIO,
                                  POS + hoverPosition.y() * WIDTH - CHESS_RATIO,
                                  CHESS_RATIO * 2, CHESS_RATIO * 2);
            }
        }
    }
    paint.end();
}

void onlinegame::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && status == GAMING && selfTurn) {
        int x = (event->pos().x() - POS + WIDTH / 2) / WIDTH;
        int y = (event->pos().y() - POS + WIDTH / 2) / WIDTH;
        if (event->pos().x() > POS - WIDTH / 2 &&
            event->pos().x() < POS + WIDTH * 14 + WIDTH / 2 &&
            event->pos().y() > POS - WIDTH / 2 &&
            event->pos().y() < POS + WIDTH * 14 + WIDTH / 2) {
            drop(x, y);
            update();
        }
    }
}

void onlinegame::mouseMoveEvent(QMouseEvent *event) {
    if (event->HoverMove && status == GAMING && selfTurn) {
        int x = (event->pos().x() - POS + WIDTH / 2) / WIDTH;
        int y = (event->pos().y() - POS + WIDTH / 2) / WIDTH;
        hoverPosition.setX(x);
        hoverPosition.setY(y);
        update();
    } else {
        hoverPosition.setX(-1);
        hoverPosition.setX(-1);
        update();
    }
}
void onlinegame::mouseDoubleClickEvent(QMouseEvent *event) {}
// Operator

void onlinegame::drop(int x, int y) {
    if (status == GAMING && selfTurn) {
        if (positionStatus[x][y] == NO_CHESS) {
            currentChess = chess(x, y, selfColor, step);
            m_board.push_back(currentChess);
            emit signalDrop();
        }
    }
}

chess onlinegame::getCurrentChess() { return currentChess; }

void onlinegame::updateBoard(int pos[15][15]) {
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            positionStatus[i][j] = pos[i][j];
        }
    }
    update();
}

void onlinegame::respondWin(int color) {
    if (color == selfColor) {
        switch (QMessageBox::question(
            this, "You win!", "Are you want to play again?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No)) {
        case QMessageBox::Yes:
            clearBoard();
            update();
            break;
        case QMessageBox::No:
            emit signalBack();
            break;
        default:
            break;
        }
    } else if (color == WHITE_CHESS) {
        switch (QMessageBox::question(
            this, "Your rival wins!", "Are you want to play again?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No)) {
        case QMessageBox::Yes:
            clearBoard();
            update();
            break;
        case QMessageBox::No:
            emit signalBack();
            break;
        default:
            break;
        }
    } else if (color == NO_CHESS) {
    }
}

void onlinegame::respondRetract() {
    switch (QMessageBox::question(
        this, "REQUEST",
        "The rival request to retract!\nDo you agree with the rival's retract?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No)) {
    case QMessageBox::Yes:
        emit signalRespondRetract(true);
        break;
    case QMessageBox::No:
        emit signalRespondRetract(false);
        break;
    default:
        break;
    }
}

void onlinegame::respondReplay() {
    switch (QMessageBox::question(this, "REQUEST",
                                  "The rival request to replay!\nDo you agree "
                                  "with the rival's requestion?",
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No)) {
    case QMessageBox::Yes:
        emit signalRespondReplay(true);
        break;
    case QMessageBox::No:
        emit signalRespondReplay(false);
        break;
    default:
        break;
    }
}
