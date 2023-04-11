#ifndef GOBANGSERVER_H_
#define GOBANGSERVER_H_
#include "./lib/openjson/openjson.h"
#include "tcpserver.h"
#include <concepts>
#include <cstring>
#include <iostream>
#include <iterator>
#include <map>
#include <netinet/in.h>
#include <ostream>
#include <string>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#define NO_CHESS 2
#define BLACK_CHESS 0
#define WHITE_CHESS 1

#define IN_LOBBY 0
#define IN_ROOM 1
#define GAMING 2
#define WATCHING 3

struct chess {
    int m_x, m_y; // 棋子位置
    int m_color;  // 颜色
    int m_step;   // 步数
    chess(int x, int y, int color, int step)
        : m_x(x), m_y(y), m_color(color), m_step(step) {}
};

struct player {        // 玩家结构体
    int m_connfd;      // 套接字
    int m_rivalConnfd; // 对手套接字
    int m_color;       // 颜色
    bool m_prepare;    // 是否准备
    int status;    // 0在大厅，1在房间，2为在准备，3为在游戏中
    int m_time;    // 颜色
    bool m_isTurn; // 是否轮到自己
    std::string m_name;
    player(int connfd, int color, bool isWatcher)
        : m_connfd(connfd), m_color(color), status(IN_LOBBY) {
        m_time = 5400;
        m_isTurn = false;
        m_name = "unknow";
        m_rivalConnfd = -1;
        m_prepare = false;
    }
    player(int connfd) : m_connfd(connfd) {
        m_rivalConnfd = -1;
        m_color = NO_CHESS;
        status = IN_LOBBY;
        m_time = 5400;
        m_isTurn = false;
        m_name = "unknow";
        m_prepare = false;
    }
};

struct room {                          // 房间结构体
    int roomID;                        // 房间ID，房间人数
    player *blackPlayer, *whitePlayer; // 玩家
    bool isGaming;                     // 是否在游戏中

    std::vector<int> watchers; // 观众

    std::vector<chess> m_board; // 存放棋子，用以悔棋复盘
    int positionStatus[15][15]; // 棋盘状态

    chess currentChess = chess(-1, -1, -1, -1);
    int turn;                              // 该谁
    int m_step;                            // 当前棋局的步数
    room() : roomID(-1), isGaming(false) { // 创建房间，玩家为1,观众为0,清空棋盘
        blackPlayer = nullptr;
        whitePlayer = nullptr;
        turn = BLACK_CHESS;
        watchers.clear();
        for (int i = 0; i < 15; i++)
            for (int j = 0; j < 15; j++)
                positionStatus[i][j] = NO_CHESS;
        m_board.clear();
    }

    int getNumOfPlayer() {
        if (blackPlayer != nullptr && whitePlayer != nullptr)
            return 2;
        else if ((blackPlayer == nullptr && whitePlayer != nullptr) ||
                 (blackPlayer != nullptr && whitePlayer == nullptr))
            return 1;
        else
            return 0;
    }
};

class gobangserver {
  private:
    open::OpenJson json;

    std::map<int, room>
        lobby; // 游戏大厅，包含多个房间，每个房间至多包含两名玩家

    char sendMsg[1024];
    int len{0};

  public:
    tcpserver tcpServer;

    std::map<int, player *> players;

    gobangserver();

    int findRival(int connfd); // 寻找对手fd

    std::map<int, room>::iterator
    getRoom(int connfd); // 根据fd寻找房间，没有则返回lobby.end()

    std::map<int, room>::iterator
    getWatchersRoom(int connfd); // 获取观战者的房间

    int getChessColor(int connfd); // 获取棋手颜色

    void sentMatchInfo(int connfd); // 发送棋盘信息
    void sentUserInfo();            // 发送玩家信息
    void sentLobbyInfo();           // 发送大厅信息
    void sentRoomInfo(int connfd);  // 发送房间信息

    void updateInfo();

    void sentInfo(int connfd);

    // void sentWatchersQuit(int connfd);

    void sentResultInfo(int connfd, int color); // 发送输赢信息

    // 接收到客户端的信号后，根据解析出的信息做出反应
    void parseInfo(int connfd, char buff[1024]); // 解析信息

    /*动作*/
    void retract(int connfd); // 悔棋操作

    void requestRetract(int connfd); // 请求悔棋

    void respondRetract(int connfd, bool anwser); // 回应悔棋

    void prepare(int connfd);

    void createRoom(int connfd); // 创建房间

    void joinRoom(int connfd, int roomID); // 加入房间

    void watchMatch(int connfd, int roomID);

    void quitRoom(int connfd); // 退出房间

    void quitLobby(int connfd); // 退出大厅

    void disconnect(int connfd);                    // 断开连接
    void drop(int connfd, int x, int y);            // 落子
    int isWin(int connfd, int x, int y, int color); // 判断输赢

    void turnToNext(int connfd) {
        if (getRoom(connfd)->second.turn == BLACK_CHESS) {
            getRoom(connfd)->second.turn = WHITE_CHESS;
        } else
            getRoom(connfd)->second.turn = BLACK_CHESS;
    }

    void restart(int connfd);
};

#endif /*GOBANGSERVER*/
