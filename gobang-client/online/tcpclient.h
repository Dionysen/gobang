#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "../lib/openjson/openjson.h"
#include "lobby.h"
#include "onlinegame.h"
#include "threadpool.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <qobject.h>
#include <string>
// #include <sys/errno.h>

#ifdef WIN32
#include <windows.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/unistd.h>
#endif

#define SERVER_IP "82.157.246.225"
// #define SERVER_IP "127.0.0.1"
#define SERVER_PORT 16556
#define BUFFSIZE 1024

class tcpclient {
  private:
    struct sockaddr_in servaddr;
#if WIN32
    unsigned long long sockfd;
#else
    int sockfd;
#endif
    open::OpenJson json;
    threadpool pool;

  public:
    tcpclient();
    int getSockFD() { return sockfd; }
    bool connectToServer(); // 连接服务器
    void closesocket();     // 关闭socket
    void disconnect();      // 断开连接

  public:
    void static parseInformation(
        onlinegame &Game, lobby &Lobby,
        std::string buff); // 接收服务端发来的信息并解析并做出反应

    // slots：
    bool joinLobby();            // 进入大厅
    void createRoom();           // 进入房间
    void joinRoom(int roomID);   // 加入房间
    void watchMatch(int roomID); // 观看比赛

    void quitLobby(); // 退出大厅，断开连接
    void quitRoom();  // 退出房间，回到大厅

    void prepared();                 // 准备
    void drop(int x, int y);         // 落子
    void requestRetract();           // 悔棋
    void repondRetract(bool anwser); // 应答悔棋
    void exit();                     // 退出
    void concede();                  // 认输
    void restartGame();              // 重新开始游戏
};

#endif // TCPCLIENT_H
