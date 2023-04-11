#ifndef GAME_H
#define GAME_H

#include "chess.h"
#include "robotthread.h"
#include "settingdialog.h"
#include <QMessageBox>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QString>
#include <QStyle>
#include <QStyleOption>
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

namespace Ui {
class game;
}

class game : public QWidget {
    Q_OBJECT

  public:
    explicit game(QWidget *parent = nullptr);
    ~game();
    bool isGaming(); // 返回游戏是否正在进行
    // online

    // 设置黑棋和白棋棋手的信息
    void setBlackTime(int blackTime);
    void setBlackName(std::string blackName);
    void setBlackId(unsigned long blackId);

    void setWhiteTime(int whiteTime);
    void setWhiteName(std::string whiteName);
    void setWhiteId(unsigned long whiteId);

    // 更新对局信息
    void setTurn(int turn);
    void setStep(int step);
    void setGameStatus(bool isGaming);

    // 更新显示的信息
    void updateRoomInfo();

    // 对局中的操作
    void newGame();              // 初始化游戏状态
    void drop(int x, int y);     // 落子
    void turnToNext();           // 换棋手
    void retract();              // 悔棋
    void replay();               // 重开
    int isWin(int x, int y);     // 判断是否结束
    void respondWin(int player); // 响应对局结束
    void concede();              // 认输

  private:
    Ui::game *ui;
    bool m_isGaming;
    int turn, step, m_whiteTime, m_blackTime;
    unsigned long m_blackPlayerId, m_whitePlayerId;
    std::string m_blackPlayerName, m_whitePlayerName;
    chess currentChess;
    std::vector<chess> m_board;
    QPoint hoverPosition;
    int positionStatus[15][15]; // 用于判断输赢

    int humanTurn, robotTurn;
    int time;
    robotthread *robotThread;

    SettingDialog *setDia;

  protected:
    void paintEvent(QPaintEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

  signals:
    void signalBackToHome();
    void signalBackNoAsk();
    void siganlRobotDrop(int x, int y);

  public slots:

    void setting();
};

#endif // GAME_H
