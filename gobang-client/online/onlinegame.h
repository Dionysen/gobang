#ifndef ONLINEGAME_H
#define ONLINEGAME_H

#include "chess.h"
#include <QMessageBox>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QString>
#include <QWidget>
#include <iostream>
#include <string>
#include <vector>

#define ROW_COLUM 15
#define POS 100
#define WIDTH 30
#define POS_SIZE 4
#define CHESS_RATIO 10
#define PLAYING 1
#define ONLINE 2
#define BLACK_CHESS 0
#define WHITE_CHESS 1
#define NO_CHESS 2

#define IN_LOBBY 0
#define IN_ROOM 1
#define GAMING 2
#define WATCHING 3

namespace Ui {
class onlinegame;
}

class onlinegame : public QWidget {
    Q_OBJECT

  public:
    explicit onlinegame(QWidget *parent = nullptr);
    ~onlinegame();

  private:
    Ui::onlinegame *ui;

    int step, m_whiteTime, m_blackTime; // 自己是什么棋子
    unsigned long m_blackPlayerId, m_whitePlayerId;
    std::string m_blackPlayerName, m_whitePlayerName;
    chess currentChess;
    std::vector<chess> m_board;
    QPoint hoverPosition;
    int positionStatus[15][15]; // 用于判断输赢

  protected:
    void paintEvent(QPaintEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

  public:
    bool selfTurn; // 是否该自己
    int selfColor; // 自己的颜色
    int status;    // 状态，在大厅或房间或游戏或观战

    void newGame();                    // 游戏初始化
    void drop(int x, int y);           // 下棋
    void updateBoard(int pos[15][15]); // 更新棋盘
    void updateRoomInfo();             // 更新房间信息
    void respondWin(int color);        // 回应棋局结束
    void respondRetract();             // 回应悔棋请求

    void setBlackId(int id) { m_blackPlayerId = id; }
    void setBlackName(std::string name) { m_blackPlayerName = name; }
    void setWhiteId(int id) { m_whitePlayerId = id; }
    void setWhiteName(std::string name) { m_whitePlayerName = name; }
    void setWatcher(bool isWatcher);

    void clearBoard() {
        for (int i = 0; i < 15; i++) {
            for (int j = 0; j < 15; j++) {
                positionStatus[i][j] = NO_CHESS;
            }
        }
        m_board.clear();
    }
    bool isEmptyBoard() {
        bool empty = true;
        for (int i = 0; i < 15; i++) {
            for (int j = 0; j < 15; j++) {
                if (positionStatus[i][j] != NO_CHESS)
                    empty = false;
            }
        }
        return empty;
    }

    void setGameStatus(bool status) { this->status = status; }
    void setSelfColor(int color) { selfColor = color; } // 设置自己的颜色
    void setselfTurn();                                 // 设置是否该自己
    chess getCurrentChess();                            // 获取当前棋子
    void turnToNext() {                                 // 改变轮次
        if (selfTurn)
            selfTurn = false;
        else
            selfTurn = true;
    }

  signals:
    void signalDrop();                      // 下棋
    void signalPrepare();                   // 准备
    void signalRequestRetract();            // 请求悔棋
    void signalRespondRetract(bool anwser); // 回应悔棋
    void signalConcede();                   // 认输
    void signalBack();                      // 返回
    void signalBackToLobby();               // 直接回到大厅
    void signalReplay();                    // 重开
    void signalRestartGame();               // 继续新开一局游戏
    void siganlUpdateUserInfo();            // 根据用户信息更新显示
};

#endif // ONLINEGAME_H
