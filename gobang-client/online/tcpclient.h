#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "../lib/openjson/openjson.h"
#include "lobby.h"
#include "onlinegame.h"

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

  public:
    tcpclient();
    int getSockFD() { return sockfd; }
    bool connectToServer(); // connect to server
    void closesocket();     // close socket
    void disconnect();      // disconnect to server

  public:
    void static parseInformation(onlinegame &Game, lobby &Lobby,
                                 std::string buff); // Receive and parse info

    // slots：
    bool joinLobby();            // join lobby
    void createRoom();           // create a room
    void joinRoom(int roomID);   // join a room
    void watchMatch(int roomID); // watch a game

    void quitLobby(); // quit lobby and disconnect to server
    void quitRoom();  // quit room and go to lobby

    void prepared();                 // prepare
    void drop(int x, int y);         // drop
    void requestRetract();           // retract
    void repondRetract(bool anwser); // respond retracting
    void requestReplay();            // replay
    void repondReplay(bool anwser);  // respond replay
    void exit();                     // exit
    void concede();                  // concede
    void restartGame();              // restart a game

    void setPlayerName(QString name);
};

#endif // TCPCLIENT_H
