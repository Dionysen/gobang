#include "onlinegame.h"
#include "chess.h"
#include "ui_onlinegame.h"
#include <iostream>
#include <ostream>
#include <qobjectdefs.h>
#include <qpushbutton.h>

onlinegame::onlinegame(QWidget *parent)
    : QWidget(parent), ui(new Ui::onlinegame) {
    ui->setupUi(this);
    this->setMouseTracking(true);
    newGame();

    connect(ui->retractBtn, &QPushButton::clicked, this,
            [=] { emit signalRequestRetract(); });

    connect(ui->concedeBtn, &QPushButton::clicked, this,
            [=] { emit signalConcede(); });

    connect(ui->replayBtn, &QPushButton::clicked, this,
            [=] { emit signalRestartGame(); });

    connect(ui->backBtn, &QPushButton::clicked, this, [=] {
        // if (isWatcher)
        emit signalBack();
    });

    connect(ui->preBtn, &QPushButton::clicked, this, [=] {
        emit signalPrepare(); // 发送准备信号
    });
}

onlinegame::~onlinegame() { delete ui; }

// 大局函数
void onlinegame::newGame() {
    status = IN_LOBBY;
    selfTurn = false;
    selfColor = NO_CHESS;
    step = 0;
    m_blackPlayerName = "Black Player";
    m_whitePlayerName = "White Player";
    m_blackPlayerId = 0;
    m_whitePlayerId = 1;
    m_blackTime = 5400;
    m_whiteTime = 5400;
    clearBoard();
    currentChess = chess(-1, -1, NO_CHESS, -1);
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

void onlinegame::updateRoomInfo() { // 更新房间信息
    // black
    ui->blackID->setText(QString::fromStdString(
        "Black ID: " + std::to_string(this->m_blackPlayerId)));
    ui->blackName->setText(
        QString::fromStdString("Black Name: " + m_blackPlayerName));
    ui->blackTime->setText(
        QString::fromStdString("Time: " + std::to_string(m_blackTime)));
    // white
    ui->whiteID->setText(QString::fromStdString(
        "White ID: " + std::to_string(this->m_whitePlayerId)));
    ui->whiteName->setText(
        QString::fromStdString("White Name: " + m_whitePlayerName));
    ui->whiteTime->setText(
        QString::fromStdString("Time: " + std::to_string(m_whiteTime)));

    if (status == GAMING ||
        status == WATCHING) { // 在线模式，且游戏开始，隐藏准备按钮
        ui->preBtn->setFixedSize(0, 0);
    } else
        ui->preBtn->setFixedSize(300, 200);
}

// 事件函数
void onlinegame::paintEvent(QPaintEvent *event) {
    QPainter paint(this); // 创建画家对象

    QColor boardBackgroundColor(206, 178, 152);
    paint.setPen(boardBackgroundColor);
    paint.setBrush(boardBackgroundColor);
    paint.drawRect(POS - WIDTH / 2, POS - WIDTH / 2, 15 * WIDTH, 15 * WIDTH);

    paint.setPen(QPen(QColor(Qt::black), 1)); // 设置画笔，黑色，宽度为1
    for (int i = 0; i < ROW_COLUM; i++) {     // 画出棋盘
        paint.drawLine(POS + i * WIDTH, POS, POS + i * WIDTH, POS + 14 * WIDTH);
    }
    for (int i = 0; i < ROW_COLUM; i++) {
        paint.drawLine(POS, POS + i * WIDTH, POS + 14 * WIDTH, POS + i * WIDTH);
    }
    // 加深边框
    paint.setPen(QPen(QColor(Qt::black), 2));
    paint.drawLine(POS, POS, POS + 14 * WIDTH, POS);
    paint.drawLine(POS, POS, POS, POS + 14 * WIDTH);
    paint.drawLine(POS + 14 * WIDTH, POS, POS + 14 * WIDTH, POS + 14 * WIDTH);
    paint.drawLine(POS, POS + 14 * WIDTH, POS + 14 * WIDTH, POS + 14 * WIDTH);
    // 加上定位点
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

    // 绘制已存在的棋子

    for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < 15; ++j) {
            if (positionStatus[i][j] == BLACK_CHESS) {
                paint.setPen(QPen(QColor(Qt::black), 1));
                paint.setBrush(Qt::black);
                paint.drawEllipse(POS + i * WIDTH - CHESS_RATIO,
                                  POS + j * WIDTH - CHESS_RATIO,
                                  CHESS_RATIO * 2, CHESS_RATIO * 2);
            } else if (positionStatus[i][j] == WHITE_CHESS) {
                paint.setPen(QPen(QColor(Qt::white), 1));
                paint.setBrush(Qt::white);
                paint.drawEllipse(POS + i * WIDTH - CHESS_RATIO,
                                  POS + j * WIDTH - CHESS_RATIO,
                                  CHESS_RATIO * 2, CHESS_RATIO * 2);
            }
        }
    }
    // 绘制悬空棋子
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
                paint.setPen(QPen(color));
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

void onlinegame::mouseReleaseEvent(QMouseEvent *event) // 检测鼠标施放信号
{
    if (event->button() == Qt::LeftButton && status == GAMING) {
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
    if (event->HoverMove && status == GAMING) {
        int x = (event->pos().x() - POS + WIDTH / 2) / WIDTH;
        int y = (event->pos().y() - POS + WIDTH / 2) / WIDTH;
        hoverPosition.setX(x);
        hoverPosition.setY(y);
        update();
    }
}

// 动作

void onlinegame::drop(int x, int y) { // 下棋
    if (status == GAMING && selfTurn) {
        if (positionStatus[x][y] == NO_CHESS) {
            currentChess = chess(x, y, selfColor, step);
            m_board.push_back(currentChess);
            emit signalDrop();
        }
    }
}

chess onlinegame::getCurrentChess() { return currentChess; } // 获取当前棋子

void onlinegame::updateBoard(int pos[15][15]) { // 更新棋盘信息
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            positionStatus[i][j] = pos[i][j];
        }
    }
    update();
}

void onlinegame::respondWin(int color) { // 回应棋局结束
    if (color == BLACK_CHESS) {
        QMessageBox mb;
        mb.setWindowTitle(tr("Black Win!"));
        mb.setText(tr("Black win!"));
        mb.setInformativeText(tr("Are you want to play again?"));
        mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No |
                              QMessageBox::Default);
        mb.setDefaultButton(QMessageBox::Yes);
        switch (mb.exec()) {
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
        QMessageBox mb;
        mb.setWindowTitle(tr("White Win!"));
        mb.setText(tr("White win!"));
        mb.setInformativeText(tr("Are you want to play again?"));
        mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No |
                              QMessageBox::Default);
        mb.setDefaultButton(QMessageBox::Yes);
        switch (mb.exec()) {
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
void onlinegame::respondRetract() { // 回应悔棋请求
    QMessageBox mb;
    mb.setWindowTitle(tr("REQUEST"));
    mb.setText(tr("The rival request to retract!"));
    mb.setInformativeText(tr("Do you agree with the rival's retract?"));
    mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    mb.setDefaultButton(QMessageBox::Yes);
    switch (mb.exec()) {
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