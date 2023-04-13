#include "game.h"
#include "ChessEngine.h"
#include "chess.h"
#include "robotthread.h"
#include "settingdialog.h"
#include "ui_game.h"
#include <iostream>
#include <qevent.h>
#include <qmessagebox.h>
#include <qnamespace.h>
#include <qobjectdefs.h>
#include <qpolygon.h>
#include <qtimer.h>
#include <string>

game::game(QWidget *parent) : QWidget(parent), ui(new Ui::game) {

    ui->setupUi(this);
    this->setMouseTracking(true);
    time = 1200;
    m_blackPlayerName = "Black player";
    m_whitePlayerName = "White player";

    // robot's initial
    updateRobotName();
    ChessEngine::beforeStart();
    robotLevel = 4;
    ChessEngine::setLevel(robotLevel);
    ChessEngine::reset(0);
    humanTurn = BLACK_CHESS;
    robotTurn = WHITE_CHESS;

    setDia = new SettingDialog(this);
    robotThread = new robotthread(this);

    // settting dialog
    connect(setDia, &SettingDialog::signalAcceptResult, this,
            [=](int diff, int color, QString time, QString name) {
                // color: white = 0, black = 1
                robotLevel = diff;
                ChessEngine::setLevel(diff + 1);
                ChessEngine::reset(color);
                humanTurn = color;
                if (color == BLACK_CHESS)
                    robotTurn = WHITE_CHESS;
                else
                    robotTurn = BLACK_CHESS;
                this->time = time.toInt();
                if (humanTurn == BLACK_CHESS) {
                    m_blackPlayerName = name.toStdString();
                } else {
                    m_whitePlayerName = name.toStdString();
                }
                emit signalSentNameToOnline(name);
                newGame();
            });

    // connection
    connect(ui->backBtn, &QPushButton::clicked, this,
            [=] { emit signalBackToHome(); });

    connect(ui->retractBtn, &QPushButton::clicked, this, [=] { retract(); });

    connect(ui->replayBtn, &QPushButton::clicked, this, [=] { replay(); });
    connect(ui->concedeBtn, &QPushButton::clicked, this, [=] { concede(); });

    // child thread
    connect(
        this, &game::siganlRobotDrop, robotThread,
        &robotthread::isTurn); // turn to robot, emit the signal, and robot drop
    connect(robotThread, &robotthread::signalSendPosition,
            this, // respond droping by robot
            [=](int x, int y) {
                drop(x, y);
                update();
                respondWin(isWin(x, y));
            });

    // timer
    connect(timer, &QTimer::timeout, this, [=] {
        if (turn == BLACK_CHESS) {
            m_blackTime--;
        } else {
            m_whiteTime--;
        }
        updateRoomInfo();
    });
    timer->start(1000);
}

game::~game() { delete ui; }

/* override event funciton */

void game::paintEvent(QPaintEvent *event) {

    QPainter paint(this); // create a painter
    QColor backgroundColor(240, 217, 195);
    QColor boardBackgroundColor(206, 178, 152);

    paint.setPen(backgroundColor);
    paint.setBrush(backgroundColor);
    // paint.drawRect(0, 0, 1000, 720);

    paint.setPen(boardBackgroundColor);
    paint.setBrush(boardBackgroundColor);
    // paint.drawRoundedRect(POS - WIDTH / 2, POS - WIDTH / 2, 15 * WIDTH,
    //                       15 * WIDTH, 2, 2);
    // paint.drawRect(POS - WIDTH / 2, POS - WIDTH / 2, 15 * WIDTH, 15 * WIDTH);
    // paint.drawRect(POS, POS, 14 * WIDTH, 14 * WIDTH);

    paint.setPen(
        QPen(QColor(Qt::black), 1));      // set pen: color: black, width: 1px
    for (int i = 0; i < ROW_COLUM; i++) { // paint chessboard
        paint.drawLine(POS + i * WIDTH, POS, POS + i * WIDTH, POS + 14 * WIDTH);
    }
    for (int i = 0; i < ROW_COLUM; i++) {
        paint.drawLine(POS, POS + i * WIDTH, POS + 14 * WIDTH, POS + i * WIDTH);
    }

    // Deepen the border
    paint.setPen(QPen(QColor(Qt::black), 2));
    paint.drawLine(POS, POS, POS + 14 * WIDTH, POS);
    paint.drawLine(POS, POS, POS, POS + 14 * WIDTH);
    paint.drawLine(POS + 14 * WIDTH, POS, POS + 14 * WIDTH, POS + 14 * WIDTH);
    paint.drawLine(POS, POS + 14 * WIDTH, POS + 14 * WIDTH, POS + 14 * WIDTH);

    // Plus anchors
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

    // paint chesses that already exist
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
    // paint chesses hovering
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
void game::mouseReleaseEvent(
    QMouseEvent *event) // Detect cursor position and operator
{
    if (event->button() == Qt::LeftButton && humanTurn == turn) {
        int x = (event->pos().x() - POS + WIDTH / 2) / WIDTH;
        int y = (event->pos().y() - POS + WIDTH / 2) / WIDTH;
        if (event->pos().x() > POS - WIDTH / 2 &&
            event->pos().x() < POS + WIDTH * 14 + WIDTH / 2 &&
            event->pos().y() > POS - WIDTH / 2 &&
            event->pos().y() < POS + WIDTH * 14 + WIDTH / 2) {
            bool isExist = false;
            if (isGaming()) {
                for (auto i = m_board.begin(); i != m_board.end(); i++) {
                    if (i->x() == x && i->y() == y)
                        isExist = true;
                }
                if (!isExist) {
                    drop(x, y);
                    update();
                    respondWin(isWin(x, y));
                    emit siganlRobotDrop(x, y);
                    robotThread->start();
                }
            }
        }
    }
}
void game::mouseDoubleClickEvent(QMouseEvent *event) {}

void game::mouseMoveEvent(QMouseEvent *event) {
    if (event->HoverMove && humanTurn == turn) {
        int x = (event->pos().x() - POS + WIDTH / 2) / WIDTH;
        int y = (event->pos().y() - POS + WIDTH / 2) / WIDTH;
        hoverPosition.setX(x);
        hoverPosition.setY(y);
        update();
    }
}

/* Interface */

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

/* Operator */
void game::newGame() // New Game
{
    m_board.clear();
    turn = BLACK_CHESS;
    step = 1;
    m_isGaming = true;
    ChessEngine::reset(humanTurn);
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
    bool isExist = false;
    if (isGaming()) {
        for (auto i = m_board.begin(); i != m_board.end(); i++) {
            if (i->x() == x && i->y() == y)
                isExist = true;
        }
        if (!isExist) {
            currentChess = chess(x, y, turn, step); // Create a new chess
            m_board.push_back(currentChess);        // add chess to board
            positionStatus[x][y] = turn;            // update position
            step++;
            turnToNext();
            updateRoomInfo();
        }
    }
}
void game::retract() // retract
{
    if (turn != humanTurn) {
        QMessageBox::critical(this, "Retract",
                              "Invalid operator!\nIt is NOT your turn!");
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
            QMessageBox::critical(this, "Retract", "The chessboard is empty!");
        }
    }
}

void game::replay() {
    switch (QMessageBox::question(
        this, "REPLAY", "Are you sure you want to replay the game?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)) {
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

int game::isWin(int x, int y) // Judge winner
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

void game::respondWin(int player) // respond win
{
    if (player == humanTurn) {
        switch (QMessageBox::question(
            this, "You win!", "Are you sure you want to play again?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)) {
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
    } else if (player == robotTurn) {
        switch (QMessageBox::question(
            this, "The robot wins!", "Are you sure you want to play again?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)) {
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
        QString::fromStdString("Time: " + std::to_string(m_blackTime / 60) +
                               ":" + to_string(m_blackTime % 60)));
    // white
    ui->whiteID->setText(QString::fromStdString(
        "White ID: " + std::to_string(this->m_whitePlayerId)));
    ui->whiteName->setText(
        QString::fromStdString("White Name: " + m_whitePlayerName));
    ui->whiteTime->setText(
        QString::fromStdString("Time: " + std::to_string(m_whiteTime / 60) +
                               ":" + to_string(m_whiteTime % 60)));

    // robot
    if (humanTurn == WHITE_CHESS) {
        ui->blackName->setText(
            QString::fromStdString("Black Name: " + level[robotLevel]));
    } else {
        ui->whiteName->setText(
            QString::fromStdString("White Name: " + level[robotLevel]));
    }

    if (turn == BLACK_CHESS) {
        ui->blackTurnPtn->show();
        ui->whiteTurnPtn->hide();
    } else {
        ui->blackTurnPtn->hide();
        ui->whiteTurnPtn->show();
    }
}

void game::setting() { setDia->show(); }

void game::updateRobotName() {
    level[0] = "Easy";
    level[1] = "Simple";
    level[2] = "Normal";
    level[3] = "Hard";
    level[4] = "Expert";
    level[5] = "Extre";
    level[6] = "Hell";
}