#ifndef GAME_H
#define GAME_H

#include "base.h"
#include "chess.h"
#include "robotthread.h"
#include "settingdialog.h"

#include <QEvent>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QTimer>
#include <QWidget>
#include <iostream>
#include <map>
#include <qevent.h>
#include <qwindowdefs.h>
#include <string>
#include <vector>

namespace Ui {
class game;
}

class game : public QWidget {
    Q_OBJECT

  public:
    explicit game(QWidget *parent = nullptr);
    ~game();
    bool isGaming(); // Return m_isGaming
    // online

    // Interface For Setting Some Information
    void setBlackTime(int blackTime);
    void setBlackName(std::string blackName);
    void setBlackId(unsigned long blackId);

    void setWhiteTime(int whiteTime);
    void setWhiteName(std::string whiteName);
    void setWhiteId(unsigned long whiteId);

    // Setting status
    void setTurn(int turn);
    void setStep(int step);
    void setGameStatus(bool isGaming);
    void updateRobotName(); // Update robot's name

    // Update information of room
    void updateRoomInfo();

    // Operator
    void newGame();              // Init a new game
    void drop(int x, int y);     // drop
    void turnToNext();           // next player
    void retract();              // retract
    void replay();               // replay
    int isWin(int x, int y);     // is someone win?
    void respondWin(int player); // respond the result
    void concede();              // concede

  private:
    Ui::game *ui;
    bool m_isGaming;
    int turn, step, m_whiteTime, m_blackTime;
    unsigned long m_blackPlayerId, m_whitePlayerId;
    std::string m_blackPlayerName, m_whitePlayerName;
    chess currentChess;
    std::vector<chess> m_board;
    QPoint hoverPosition;
    int positionStatus[15][15]; // chessboard position status

    int humanTurn, robotTurn;
    int robotLevel;
    int time;
    robotthread *robotThread;

    SettingDialog *setDia;

    QTimer *timer = new QTimer(this);

    std::map<int, std::string> level;

  protected:
    void paintEvent(QPaintEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

  signals:
    void signalBackToHome();
    void signalBackNoAsk();
    void siganlRobotDrop(int x, int y);
    void signalSentNameToOnline(QString name);

  public slots:

    void setting();
};

#endif // GAME_H
