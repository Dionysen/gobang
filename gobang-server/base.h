#include <iostream>
#include <map>
#include <vector>

#define NO_CHESS 2
#define BLACK_CHESS 0
#define WHITE_CHESS 1

#define IN_LOBBY 0
#define IN_ROOM 1
#define GAMING 2
#define WATCHING 3

struct chess {
    int m_x, m_y; // position
    int m_color;  // color
    int m_step;   // step
    chess(int x, int y, int color, int step)
        : m_x(x), m_y(y), m_color(color), m_step(step) {}
};

struct player {        // player struct
    int m_connfd;      // socket
    int m_rivalConnfd; // rival socket
    int m_color;       // color
    bool m_prepare;    // is prepare
    int status;        // macro
    int m_time;        // time
    bool m_isTurn;     // turn to self?
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

struct room {                          // room
    int roomID;                        // room ID
    player *blackPlayer, *whitePlayer; // two players
    bool isGaming;                     // is gaming

    std::vector<int> watchers; // watchers

    std::vector<chess> m_board; // board vector
    int positionStatus[15][15]; // board array

    chess currentChess = chess(-1, -1, -1, -1);
    int turn;                              // turn
    int m_step;                            // step
    room() : roomID(-1), isGaming(false) { // constructor
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
