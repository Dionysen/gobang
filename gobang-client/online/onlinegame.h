#ifndef ONLINEGAME_H
#define ONLINEGAME_H

#include "base.h"
#include "chess.h"

#include <QMessageBox>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <iostream>
#include <qtimer.h>
#include <string>
#include <vector>

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

    int step, m_whiteTime, m_blackTime;
    unsigned long m_blackPlayerId, m_whitePlayerId;
    std::string m_name, m_blackPlayerName, m_whitePlayerName;
    chess currentChess;
    std::vector<chess> m_board;
    QPoint hoverPosition;
    int positionStatus[15][15];

    QTimer *timer = new QTimer(this);

  protected:
    void paintEvent(QPaintEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

  public:
    bool selfTurn; // is turn to self
    int selfColor; // self color
    int status;    // status, GAMING, IN_LOBBY, IN_ROOM or WATCHING

    void newGame();                    // Init
    void drop(int x, int y);           // Drop
    void updateBoard(int pos[15][15]); // Update board
    void updateRoomInfo();             // Update room information
    void respondWin(int color);        // Respond win
    void respondRetract();             // Respond requestion for restracting
    void respondReplay();              // Respond requestion for replay

    void setBlackId(int id) { m_blackPlayerId = id; }
    void setBlackName(std::string name) { m_blackPlayerName = name; }
    void setWhiteId(int id) { m_whitePlayerId = id; }
    void setWhiteName(std::string name) { m_whitePlayerName = name; }
    void setWatcher(bool isWatcher);
    void setSelfName(std::string name) { m_name = name; }

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
    void setSelfColor(int color) { selfColor = color; } // set selfcolor
    void setselfTurn();                                 // set selfturn
    chess getCurrentChess();                            // get current chess
    void setCurrentChess(chess c) {
        currentChess.setX(c.x());
        currentChess.setY(c.y());
    }
    int getTurn() {
        if (selfColor == BLACK_CHESS) {
            if (selfTurn)
                return BLACK_CHESS;
            else
                return WHITE_CHESS;
            ;
        } else {
            if (selfTurn)
                return WHITE_CHESS;
            else
                return BLACK_CHESS;
            ;
        }
    }

    void turnToNext() { // next
        if (selfTurn)
            selfTurn = false;
        else
            selfTurn = true;
    }

  signals:
    void signalDrop();                      // Drop
    void signalPrepare();                   // Prepare
    void signalRequestRetract();            // Request retracting
    void signalRespondRetract(bool anwser); // Respond retracting
    void signalRequestReplay();             // Request replay
    void signalRespondReplay(bool anwser);  // Respond replay
    void signalConcede();                   // Concede
    void signalBack();                      // Back
    void signalBackToLobby();               // Back to lobby
    void signalRestartGame();               // Restart a new game
    void siganlUpdateUserInfo();            // Update user info
};

#endif // ONLINEGAME_H
