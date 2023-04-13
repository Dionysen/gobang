#ifndef GOBANGSERVER_H_
#define GOBANGSERVER_H_

#include "./lib/openjson/openjson.h"
#include "base.h"
#include "tcpserver.h"
#include <string>

class gobangserver {
  private:
    open::OpenJson json;

    std::map<int, room> lobby; // lobby, contains many rooms

    char sendMsg[1024];
    int len{0};

  public:
    tcpserver tcpServer;

    std::map<int, player *> players;

    gobangserver();

    int findRival(int connfd); // find rival's socket

    std::map<int, room>::iterator
    getRoom(int connfd); // find a room that has the connfd

    std::map<int, room>::iterator
    getWatchersRoom(int connfd); // find a room that has the watcher

    int getChessColor(int connfd); // get a player's color

    void sentMatchInfo(int connfd); // sent Match Info
    void sentUserInfo();            // sent user info
    void sentLobbyInfo();           // sent lobby info
    void sentRoomInfo(int connfd);  // sent room info

    void updateInfo();

    void sentInfo(int connfd);

    // void sentWatchersQuit(int connfd);

    void sentResultInfo(int connfd, int color); // sent result
    void sentCurrentChess(int connfd, chess c);

    // After receiving the signal from the client, it reacts according to the
    // parsed information
    void parseInfo(int connfd, char buff[1024]); // parse info

    /* Operator */
    void retract(int connfd);

    void requestRetract(int connfd);
    void requestReplay(int connfd);

    void respondRetract(int connfd, bool anwser);
    void respondReplay(int connfd, bool anwser);

    void prepare(int connfd);

    void createRoom(int connfd);

    void joinRoom(int connfd, int roomID);

    void watchMatch(int connfd, int roomID);

    void quitRoom(int connfd);

    void quitLobby(int connfd);

    void setPlayerName(int connfd, std::string name);

    void disconnect(int connfd);
    void drop(int connfd, int x, int y);
    int isWin(int connfd, int x, int y, int color);

    void turnToNext(int connfd);

    void restart(int connfd);
};

#endif /*GOBANGSERVER*/
