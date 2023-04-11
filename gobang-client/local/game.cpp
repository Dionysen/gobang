#include "game.h"
#include "ChessEngine.h"
#include "chess.h"
#include "robotthread.h"
#include "settingdialog.h"
#include "ui_game.h"
#include <iostream>
#include <qbrush.h>
#include <qcolor.h>
#include <qevent.h>
#include <qmessagebox.h>
#include <qobjectdefs.h>
#include <qpen.h>
#include <string>
#include <tuple>

game::game(QWidget *parent) : QWidget(parent), ui(new Ui::game) {

    ui->setupUi(this);
    this->setMouseTracking(true);

    setDia = new SettingDialog(this);

    ChessEngine::beforeStart();
    ChessEngine::setLevel(4);
    ChessEngine::reset(0);
    humanTurn = BLACK_CHESS;

    robotThread = new robotthread(this);

    // dialog
    connect(setDia, &SettingDialog::signalAcceptResult, this,
            [=](int diff, int color, QString time) {
                // color 白色是0,黑色是1
                ChessEngine::setLevel(diff + 1);
                ChessEngine::reset(color);
                humanTurn = color;
                robotTurn = color % 1;
                this->time = time.toInt();
                newGame();
            });

    // 按钮信号连接
    connect(ui->backBtn, &QPushButton::clicked, this,
            [=] { emit signalBackToHome(); });

    connect(ui->retractBtn, &QPushButton::clicked, this, [=] { retract(); });

    connect(ui->replayBtn, &QPushButton::clicked, this, [=] { replay(); });
    connect(ui->concedeBtn, &QPushButton::clicked, this, [=] { concede(); });

    // 子线程
    connect(this, &game::siganlRobotDrop, robotThread,
            &robotthread::isTurn); // 轮到机器人，发送信号，调用isTurn下棋
    connect(robotThread, &robotthread::signalSendPosition,
            this, // 响应机器人下棋结束
            [=](int x, int y) {
                drop(x, y);
                update();
                respondWin(isWin(x, y));
            });
}

game::~game() { delete ui; }

/* 重写事件函数 */
void game::paintEvent(QPaintEvent *event) {

    QPainter paint(this); // 创建画家对象
    QColor backgroundColor(240, 217, 195);
    QColor boardBackgroundColor(206, 178, 152);

    paint.setPen(backgroundColor);
    paint.setBrush(backgroundColor);
    // paint.drawRect(0, 0, 1000, 720);

    paint.setPen(boardBackgroundColor);
    paint.setBrush(boardBackgroundColor);
    paint.drawRoundedRect(POS - WIDTH / 2, POS - WIDTH / 2, 15 * WIDTH,
                          15 * WIDTH, 2, 2);
    // paint.drawRect(POS - WIDTH / 2, POS - WIDTH / 2, 15 * WIDTH, 15 * WIDTH);
    // paint.drawRect(POS, POS, 14 * WIDTH, 14 * WIDTH);

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
        hoverPosition.y() >= 0 && hoverPosition.y() < 15) {
        bool isExist = false;
        for (auto i = m_board.begin(); i != m_board.end(); i++) {
            if (i->x() == hoverPosition.x() && i->y() == hoverPosition.y())
                isExist = true;
        }
        if (!isExist) {
            if (turn == WHITE_CHESS) {
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
void game::mouseReleaseEvent(QMouseEvent *event) // 检测鼠标施放信号
{
    if (event->button() == Qt::LeftButton && humanTurn == turn) {
        int x = (event->pos().x() - POS + WIDTH / 2) / WIDTH;
        int y = (event->pos().y() - POS + WIDTH / 2) / WIDTH;
        if (event->pos().x() > POS - WIDTH / 2 &&
            event->pos().x() < POS + WIDTH * 14 + WIDTH / 2 &&
            event->pos().y() > POS - WIDTH / 2 &&
            event->pos().y() < POS + WIDTH * 14 + WIDTH / 2) {
            drop(x, y);
            update();
            respondWin(isWin(x, y));
            emit siganlRobotDrop(x, y);
            robotThread->start();
        }
    }
}

void game::mouseMoveEvent(QMouseEvent *event) {
    if (event->HoverMove && humanTurn == turn) {
        int x = (event->pos().x() - POS + WIDTH / 2) / WIDTH;
        int y = (event->pos().y() - POS + WIDTH / 2) / WIDTH;
        hoverPosition.setX(x);
        hoverPosition.setY(y);
        update();
    }
}

/* 接口 */

bool game::isGaming() { return m_isGaming; }

void game::setBlackTime(int blackTime) { m_blackTime = blackTime; }
void game::setBlackName(std::string blackName) {
    m_blackPlayerName = blackName;
}
void game::setBlackId(unsigned long blackId) { m_blackPlayerId = blackId; }

void game::setWhiteTime(int whiteTime) { m_whiteTime = whiteTime; }
void game::setWhiteName(std::string whiteName) {
    m_whitePlayerName = whiteName;
}
void game::setWhiteId(unsigned long whiteId) { m_whitePlayerId = whiteId; }

void game::setTurn(int turn) { this->turn = turn; }
void game::setStep(int step) { this->step = step; }
void game::setGameStatus(bool isGaming) { m_isGaming = isGaming; }

/* 对局操作 */
void game::newGame() // 新对局，初始化棋局
{
    m_board.clear();
    turn = BLACK_CHESS;
    step = 1;
    m_isGaming = true;
    m_blackPlayerName = "Black Player";
    m_whitePlayerName = "White Player";
    m_blackTime = time;
    m_whiteTime = time;
    currentChess = chess(-1, -1, -1, -1);
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            positionStatus[i][j] = NO_CHESS;
        }
    }
    updateRoomInfo();
    if (humanTurn == WHITE_CHESS) {
        drop(ChessEngine::getLastPosition().x,
             ChessEngine::getLastPosition().y);
    }
}

void game::drop(int x, int y) {
    if (isGaming()) {
        bool isExist = false;
        for (auto i = m_board.begin(); i != m_board.end(); i++) {
            if (i->x() == x && i->y() == y)
                isExist = true;
        }
        if (!isExist) {
            currentChess = chess(x, y, turn, step); // 创建新的棋子
            m_board.push_back(currentChess);        // 向棋盘中添加棋子
            positionStatus[x][y] = turn;            // 更新位置信息
            step++;
            turnToNext();
        }
    }
}
void game::retract() // 悔棋
{
    if (turn != humanTurn) {
        QMessageBox mb;
        mb.setWindowTitle(tr("Retract"));
        mb.setText(tr("Invalid operator!"));
        mb.setInformativeText(tr("It is NOT your turn!"));
        mb.setStandardButtons(QMessageBox::Ok);
        mb.setDefaultButton(QMessageBox::Yes);
        switch (mb.exec()) {
        case QMessageBox::Yes:
            break;
        default:
            break;
        }
    } else {
        if (!m_board.empty() && isGaming()) {
            currentChess = m_board.back();
            positionStatus[currentChess.x()][currentChess.y()] = NO_CHESS;
            m_board.pop_back();
            update();
            ChessEngine::takeBack();
            currentChess = m_board.back();
            positionStatus[currentChess.x()][currentChess.y()] = NO_CHESS;
            m_board.pop_back();
            update();
        } else {
            QMessageBox mb;
            mb.setWindowTitle(tr("Retract"));
            mb.setText(tr("Invalid operator!"));
            mb.setInformativeText(tr("The chessboard is empty!"));
            mb.setStandardButtons(QMessageBox::Ok);
            mb.setDefaultButton(QMessageBox::Yes);
            switch (mb.exec()) {
            case QMessageBox::Yes:
                break;
            default:
                break;
            }
        }
    }
}

void game::replay() {
    QMessageBox mb;
    mb.setWindowTitle(tr("REPLAY"));
    mb.setText(tr("Replay the game!"));
    mb.setInformativeText(tr("Are you sure?"));
    mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    mb.setDefaultButton(QMessageBox::Yes);
    switch (mb.exec()) {
    case QMessageBox::Yes:
        newGame();
        update();
        break;
    case QMessageBox::No:
        break;
    default:
        break;
    }
}
void game::concede() {
    if (humanTurn == BLACK_CHESS) {
        respondWin(WHITE_CHESS);
        update();
    } else {
        respondWin(BLACK_CHESS);
        update();
    }
}
void game::turnToNext() {
    if (turn == BLACK_CHESS)
        turn = WHITE_CHESS;
    else
        turn = BLACK_CHESS;
}

int game::isWin(int x, int y) // 判断输赢
{
    for (int i = -4; i <= 0; i++) {
        if (positionStatus[x + i][y] == currentChess.getColor() &&
            positionStatus[x + i + 1][y] == currentChess.getColor() &&
            positionStatus[x + i + 2][y] == currentChess.getColor() &&
            positionStatus[x + i + 3][y] == currentChess.getColor() &&
            positionStatus[x + i + 4][y] == currentChess.getColor()) {
            return currentChess.getColor();
        }
    }
    for (int i = -4; i <= 0; i++) {
        if (positionStatus[x][y + i] == currentChess.getColor() &&
            positionStatus[x][y + i + 1] == currentChess.getColor() &&
            positionStatus[x][y + i + 2] == currentChess.getColor() &&
            positionStatus[x][y + i + 3] == currentChess.getColor() &&
            positionStatus[x][y + i + 4] == currentChess.getColor()) {
            return currentChess.getColor();
        }
    }
    for (int i = -4; i <= 0; i++) {
        if (positionStatus[x + i][y + i] == currentChess.getColor() &&
            positionStatus[x + i + 1][y + i + 1] == currentChess.getColor() &&
            positionStatus[x + i + 2][y + i + 2] == currentChess.getColor() &&
            positionStatus[x + i + 3][y + i + 3] == currentChess.getColor() &&
            positionStatus[x + i + 4][y + i + 4] == currentChess.getColor()) {
            return currentChess.getColor();
        }
    }
    for (int i = -4; i <= 0; i++) {
        if (positionStatus[x - i][y + i] == currentChess.getColor() &&
            positionStatus[x - i - 1][y + i + 1] == currentChess.getColor() &&
            positionStatus[x - i - 2][y + i + 2] == currentChess.getColor() &&
            positionStatus[x - i - 3][y + i + 3] == currentChess.getColor() &&
            positionStatus[x - i - 4][y + i + 4] == currentChess.getColor()) {
            return currentChess.getColor();
        }
    }
    return NO_CHESS;
}

void game::respondWin(int player) // 响应输赢
{

    if (player == BLACK_CHESS) {
        QMessageBox mb;
        mb.setWindowTitle(tr("Black Win!"));
        mb.setText(tr("Black win!"));
        mb.setInformativeText(tr("Are you want to play again?"));
        mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No |
                              QMessageBox::Default);
        mb.setDefaultButton(QMessageBox::Yes);
        switch (mb.exec()) {
        case QMessageBox::Yes:
            ChessEngine::reset(humanTurn);
            newGame();
            break;
        case QMessageBox::No:
            emit signalBackNoAsk();
            break;
        default:
            break;
        }
    } else if (player == WHITE_CHESS) {
        QMessageBox mb;
        mb.setWindowTitle(tr("White Win!"));
        mb.setText(tr("White win!"));
        mb.setInformativeText(tr("Are you want to play again?"));
        mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No |
                              QMessageBox::Default);
        mb.setDefaultButton(QMessageBox::Yes);
        switch (mb.exec()) {
        case QMessageBox::Yes:
            ChessEngine::reset(humanTurn);
            newGame();
            break;
        case QMessageBox::No:
            emit signalBackNoAsk();
            break;
        default:
            break;
        }
    } else if (player == NO_CHESS) {
    }
}

void game::updateRoomInfo() {
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
}

void game::setting() { setDia->show(); }