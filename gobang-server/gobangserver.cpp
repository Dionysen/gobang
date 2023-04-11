#include "gobangserver.h"
#include "./lib/openjson/openjson.h"
#include "tcpserver.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iterator>
#include <map>
#include <netinet/in.h>
#include <ostream>
#include <sched.h>
#include <string>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <vector>

gobangserver::gobangserver() { tcpServer.initServer(DEFAULT_PORT); }

int gobangserver::findRival(int connfd) { // 查找对手的套接字,没有对手时返回-1
    return players.at(connfd)->m_rivalConnfd;
}

std::map<int, room>::iterator gobangserver::getRoom(
    int connfd) { // 返回值的first为房间ID,second为玩家，找不到时返回lobby.end()
    for (auto i = lobby.begin(); i != lobby.end(); i++) {
        if (i->second.blackPlayer != nullptr)
            if (i->second.blackPlayer->m_connfd == connfd)
                return i;
        if (i->second.whitePlayer != nullptr)
            if (i->second.whitePlayer->m_connfd == connfd) {
                return i;
            }
    }
    return lobby.end();
}

std::map<int, room>::iterator gobangserver::getWatchersRoom(
    int connfd) { // 寻找观战者的房间，找不到返回lobby.end()
    for (auto i = lobby.begin(); i != lobby.end(); i++) {
        for (auto j = i->second.watchers.begin(); j != i->second.watchers.end();
             j++) {
            if (connfd == *j) {
                return i;
            }
        }
    }
    return lobby.end();
}

int gobangserver::getChessColor(int connfd) {
    return players.at(connfd)->m_color;
}

void gobangserver::updateInfo() { // 从玩家信息，更新到大厅和房间信息
    for (auto i : players) {
        switch (i.second->status) {
        case IN_LOBBY:
            break;
        case IN_ROOM:
            if (findRival(i.first) != -1) {
                if (getRoom(i.first)->second.blackPlayer->m_prepare &&
                    getRoom(i.first)->second.whitePlayer->m_prepare) {
                    getRoom(i.first)->second.isGaming = true;
                    i.second->status = GAMING;
                }
            }
            break;
        case GAMING:
            if (findRival(i.first) != -1) {
                if (!getRoom(i.first)->second.blackPlayer->m_prepare ||
                    !getRoom(i.first)->second.whitePlayer->m_prepare) {
                    getRoom(i.first)->second.isGaming = false;
                    i.second->status = IN_ROOM;
                }
            } else {
                getRoom(i.first)->second.isGaming = false;
                i.second->status = IN_ROOM;
            }
            break;
        case WATCHING:
            break;
        default:
            break;
        }
    }
}

void gobangserver::sentInfo(int connfd) {
    updateInfo();    // 更新游戏状态
    sentLobbyInfo(); // 向所有人发送大厅信息
    sentUserInfo();  // 向所有用户发送自己的信息
    sentRoomInfo(connfd); // 如果用户在房间，则发送房间信息到选手双方和所有观众
    sentMatchInfo(connfd); // 如果用户在房间，则发送对战信息到选手双方和所有观众
}

void gobangserver::sentLobbyInfo() { // 发送大厅信息
    std::string buff{};
    buff.clear();

    memset(sendMsg, 0, sizeof(sendMsg));

    json.clear();
    json["head"] = "lobby";
    json["nums"] = lobby.size();
    auto &nodeLobby = json["lobby"];
    int i = 0;
    auto j = lobby.begin();
    for (j = lobby.begin(), i = 0; j != lobby.end(); i++, j++) {
        nodeLobby[i][0] = j->second.roomID;
        nodeLobby[i][1] = j->second.getNumOfPlayer();
        nodeLobby[i][2] = j->second.watchers.size();
        nodeLobby[i][3] = j->second.isGaming;
    }

    buff = json.encode();
    len = strlen(buff.c_str());
    memcpy(sendMsg, &len, sizeof(int));
    memcpy(sendMsg + sizeof(int), buff.c_str(), len);

    std::cout << "服务器: 发送大厅信息： " << (std::string)sendMsg << std::endl;
    for (auto i : players) // 向所有人发送大厅信息
    {
        if (-1 == send(i.second->m_connfd, sendMsg,
                       strlen(buff.c_str()) + sizeof(int), 0)) {
            std::cout << "Failed to sent info\n";
        }
    }
}

// 除了大厅信息，所有函数都只向一个客户端发送信息

void gobangserver::sentMatchInfo(int connfd) { // 发送棋盘信息
                                               // 是玩家或房间号
    std::string buff{};
    buff.clear();

    memset(sendMsg, 0, sizeof(sendMsg));

    json.clear();

    if (players[connfd]->status == IN_LOBBY) { // 在大厅
        // do nothing
    } else if (players[connfd]->status == IN_ROOM ||
               players[connfd]->status == GAMING) { // 是玩家
        json["head"] = "board";
        json["connfd"] = connfd;
        json["turn"] = getRoom(connfd)->second.turn; // 轮次
        auto &nodeBoard = json["board"];
        for (int i = 0; i < 15; i++) {
            for (int j = 0; j < 15; j++) {
                auto &node = nodeBoard[i];
                node[std::to_string(i)][j] =
                    getRoom(connfd)->second.positionStatus[i][j];
            }
        }
        // 玩家可能会改变，因此给所有需要的人更新棋盘信息
        buff = json.encode();

        len = strlen(buff.c_str());
        memcpy(sendMsg, &len, sizeof(int));
        memcpy(sendMsg + sizeof(int), buff.c_str(), len);

        send(connfd, sendMsg, strlen(buff.c_str()) + sizeof(int), 0);
        if (findRival(connfd) != -1)
            send(findRival(connfd), sendMsg, strlen(buff.c_str()) + sizeof(int),
                 0);
        for (auto i = 0; i < getRoom(connfd)->second.watchers.size(); i++) {
            send(getRoom(connfd)->second.watchers[i], sendMsg,
                 strlen(buff.c_str()) + sizeof(int), 0);
        }
    } else if (players[connfd]->status == WATCHING) { // 是观众
        json["head"] = "board";
        json["connfd"] = connfd;
        json["turn"] = WATCHING; // 观众轮次是3
        auto &nodeBoard = json["board"];
        for (int i = 0; i < 15; i++) {
            for (int j = 0; j < 15; j++) {
                auto &node = nodeBoard[i];
                node[std::to_string(i)][j] =
                    getWatchersRoom(connfd)->second.positionStatus[i][j];
            }
        }
        // 观众不会改变棋盘状态，原路返回即可
        buff = json.encode();

        len = strlen(buff.c_str());
        memcpy(sendMsg, &len, sizeof(int));
        memcpy(sendMsg + sizeof(int), buff.c_str(), len);

        send(connfd, sendMsg, strlen(buff.c_str()) + sizeof(int), 0);
    }
    std::cout << "服务器: 发送棋盘信息： " << sendMsg << std::endl;
}

void gobangserver::sentUserInfo() { // 向单个客户端发送用户信息

    std::string buff{};
    buff.clear();

    memset(sendMsg, 0, sizeof(sendMsg));

    for (auto i : players) {
        json.clear();
        json["head"] = "user";
        json["connfd"] = i.first;

        switch (i.second->status) {
        case IN_LOBBY:
            json["status"] = IN_LOBBY;
            break;
        case IN_ROOM:
            json["status"] = IN_ROOM;
            break;
        case GAMING:
            json["status"] = GAMING;
            if (getRoom(i.first) != lobby.end() &&
                players[i.first]->m_color == BLACK_CHESS) {
                json["selfColor"] = BLACK_CHESS;
                json["selfTurn"] =
                    (BLACK_CHESS == getRoom(i.first)->second.turn);

            } else if (getRoom(i.first) != lobby.end() &&
                       players[i.first]->m_color == WHITE_CHESS) {
                json["selfColor"] = WHITE_CHESS;
                json["selfTurn"] =
                    (WHITE_CHESS == getRoom(i.first)->second.turn);
            }
            break;
        case WATCHING:
            json["status"] = WATCHING;

            break;
        default:
            break;
        }
        buff = json.encode();

        len = strlen(buff.c_str());
        memcpy(sendMsg, &len, sizeof(int));
        memcpy(sendMsg + sizeof(int), buff.c_str(), len);

        std::cout << "服务器: 发送用户信息： " << sendMsg << std::endl;
        send(i.first, sendMsg, strlen(buff.c_str()) + sizeof(int), 0);
    }
}

void gobangserver::sentRoomInfo(int connfd) { // 发送房间内玩家信息

    std::string buff{};
    buff.clear();

    memset(sendMsg, 0, sizeof(sendMsg));

    json.clear();
    json["head"] = "room";
    json["connfd"] = connfd;
    auto &nodeRoom = json["room"];

    if (players[connfd]->status == WATCHING) { // 在观众席
        if (getWatchersRoom(connfd)->second.blackPlayer != nullptr &&
            getWatchersRoom(connfd)->second.whitePlayer != nullptr) {
            nodeRoom["blackID"] =
                getWatchersRoom(connfd)->second.blackPlayer->m_connfd;
            nodeRoom["blackName"] =
                getWatchersRoom(connfd)->second.blackPlayer->m_name;
            nodeRoom["whiteID"] =
                getWatchersRoom(connfd)->second.whitePlayer->m_connfd;
            nodeRoom["whiteName"] =
                getWatchersRoom(connfd)->second.whitePlayer->m_name;
        } else if (getWatchersRoom(connfd)->second.blackPlayer != nullptr &&
                   getWatchersRoom(connfd)->second.whitePlayer == nullptr) {
            nodeRoom["blackID"] =
                getWatchersRoom(connfd)->second.blackPlayer->m_connfd;
            nodeRoom["blackName"] =
                getWatchersRoom(connfd)->second.blackPlayer->m_name;
            nodeRoom["whiteID"] = -1;
            nodeRoom["whiteName"] = "None";
        } else if (getWatchersRoom(connfd)->second.blackPlayer == nullptr &&
                   getWatchersRoom(connfd)->second.whitePlayer != nullptr) {
            nodeRoom["blackID"] = -1;
            nodeRoom["blackName"] = "None";
            nodeRoom["whiteID"] =
                getWatchersRoom(connfd)->second.whitePlayer->m_connfd;
            nodeRoom["whiteName"] =
                getWatchersRoom(connfd)->second.whitePlayer->m_name;
        }

        buff = json.encode();
        len = strlen(buff.c_str());
        memcpy(sendMsg, &len, sizeof(int));
        memcpy(sendMsg + sizeof(int), buff.c_str(), len);

        send(connfd, sendMsg, strlen(buff.c_str()) + sizeof(int),
             0); // 观众的消息不会改变房间内部显示状态，因此只需原路发回

    } else if (players[connfd]->status == IN_ROOM ||
               players[connfd]->status == GAMING) { // 在房间中或游戏中
        if (getRoom(connfd)->second.blackPlayer != nullptr &&
            getRoom(connfd)->second.whitePlayer != nullptr) {
            nodeRoom["blackID"] = getRoom(connfd)->second.blackPlayer->m_connfd;
            nodeRoom["blackName"] = getRoom(connfd)->second.blackPlayer->m_name;
            nodeRoom["whiteID"] = getRoom(connfd)->second.whitePlayer->m_connfd;
            nodeRoom["whiteName"] = getRoom(connfd)->second.whitePlayer->m_name;
        } else if (getRoom(connfd)->second.blackPlayer != nullptr &&
                   getRoom(connfd)->second.whitePlayer == nullptr) {
            nodeRoom["blackID"] = getRoom(connfd)->second.blackPlayer->m_connfd;
            nodeRoom["blackName"] = getRoom(connfd)->second.blackPlayer->m_name;
            nodeRoom["whiteID"] = -1;
            nodeRoom["whiteName"] = "None";
        } else if (getRoom(connfd)->second.blackPlayer == nullptr &&
                   getRoom(connfd)->second.whitePlayer != nullptr) {
            nodeRoom["blackID"] = -1;
            nodeRoom["blackName"] = "None";
            nodeRoom["whiteID"] = getRoom(connfd)->second.whitePlayer->m_connfd;
            nodeRoom["whiteName"] = getRoom(connfd)->second.whitePlayer->m_name;
        }

        buff =
            json.encode(); // 玩家可能会改变房间内信息，因此向每个观众更新房间内信息

        len = strlen(buff.c_str());
        memcpy(sendMsg, &len, sizeof(int));
        memcpy(sendMsg + sizeof(int), buff.c_str(), len);
        send(connfd, sendMsg, strlen(buff.c_str()) + sizeof(int), 0);
        if (findRival(connfd) != -1)
            send(findRival(connfd), sendMsg, strlen(buff.c_str()) + sizeof(int),
                 0);
        for (auto i = 0; i < getRoom(connfd)->second.watchers.size(); i++) {
            send(getRoom(connfd)->second.watchers[i], sendMsg,
                 strlen(buff.c_str()) + sizeof(int), 0);
        }
    }

    std::cout << "服务器: 发送房间信息： " << sendMsg << std::endl;
}

void gobangserver::sentResultInfo(int connfd,
                                  int color) { // 向connfd发送对局结果

    if (getRoom(connfd)->second.isGaming) {
        std::string buff{};
        buff.clear();
        json.clear();
        json["head"] = "result";
        json["connfd"] = connfd;
        json["winner"] = color;
        buff = json.encode();

        memset(sendMsg, 0, sizeof(sendMsg));

        len = strlen(buff.c_str());
        memcpy(sendMsg, &len, sizeof(int));
        memcpy(sendMsg + sizeof(int), buff.c_str(), len);
        send(connfd, sendMsg, strlen(buff.c_str()) + sizeof(int), 0);
        std::cout << "服务器: 发送结果信息： " << buff << std::endl;
    }
}

// 接收到客户端的信号后，根据解析出的信息做出反应
void gobangserver::parseInfo(
    int connfd,
    char buff[1024]) { // 解析信息，根据来信内容做出反应

    open::OpenJson json;
    json.decode(buff);

    std::cout << "客户端: " << connfd << " 发送信息： " << buff << std::endl;

    std::string action = json["action"].s();

    if (action.compare("createRoom") == 0) { // 信号为创建房间
        createRoom(connfd);
    } else if (action.compare("toLobby") == 0) { // 信号为进入大厅
        players[connfd]->m_connfd = connfd;
    } else if (action.compare("joinRoom") == 0) { // 信号为加入房间
        joinRoom(connfd, json["roomID"].i32());
    } else if (action.compare("watchMatch") == 0) { // 信号为观看比赛
        watchMatch(connfd, json["roomID"].i32());
    } else if (action.compare("quitRoom") == 0) { // 信号为退出房间
        quitRoom(connfd);
    } else if (action.compare("quitLobby") == 0) { // 信号为退出大厅
        quitLobby(connfd);
    } else if (action.compare("restart") == 0) { // 信号为开始新的一局
        restart(connfd);
    } else if (action.compare("prepare") == 0) { // 信号为准备
        prepare(connfd);
    } else if (action.compare("drop") == 0) { // 信号为落子
        int x{json["x"].i32()};
        int y{json["y"].i32()};
        drop(connfd, x, y);
    } else if (action.compare("request") == 0) {      // 信号为请求
        if (json["type"].s().compare("retract") == 0) // 请求悔棋
            // 发送悔棋请求
            requestRetract(connfd);
    } else if (action.compare("respond") == 0) {      // 信号为回应
        if (json["type"].s().compare("retract") == 0) // 回应悔棋
            respondRetract(connfd, json["anwser"].b());
    } else if (action.compare("concede") == 0) { // 信号为认输
        if (getRoom(connfd)->second.whitePlayer->m_connfd == connfd) {
            sentResultInfo(connfd, BLACK_CHESS);
            sentResultInfo(findRival(connfd), BLACK_CHESS);
            restart(connfd);
        } else {
            sentResultInfo(connfd, BLACK_CHESS);
            sentResultInfo(findRival(connfd), BLACK_CHESS);
            restart(connfd);
        }
    } else if (action.compare("exit") == 0) { // 信号为退出（视为认输
    } else {
        std::cout << "Parse error!!!\n";
    }
    sentInfo(connfd);
}

// 动作
void gobangserver::retract(int connfd) { // 悔棋操作
    if (!getRoom(connfd)->second.m_board.empty()) {
        std::cout << "in retract\n";
        getRoom(connfd)->second.m_board.pop_back();
        for (int i = 0; i < 15; i++) {
            for (int j = 0; j < 15; j++)
                getRoom(connfd)->second.positionStatus[i][j] = NO_CHESS;
        }
        for (auto i : getRoom(connfd)->second.m_board) {
            getRoom(connfd)->second.positionStatus[i.m_x][i.m_y] = i.m_color;
        }
        turnToNext(connfd);
    }
}
void gobangserver::requestRetract(int connfd) { // 请求悔棋
    std::string buff{};
    json.clear();
    json["head"] = "request";
    json["type"] = "retract";
    buff = json.encode();

    memset(sendMsg, 0, sizeof(sendMsg));
    len = strlen(buff.c_str());
    memcpy(sendMsg, &len, sizeof(int));
    memcpy(sendMsg + sizeof(int), buff.c_str(), len);
    send(findRival(connfd), sendMsg, strlen(buff.c_str()) + sizeof(int), 0);
    std::cout << "[Send request Info: " << buff << std::endl;
}

void gobangserver::respondRetract(int connfd, bool anwser) { // 回应悔棋
    if (anwser) {
        retract(connfd);
        sentMatchInfo(connfd);
        std::cout << "Retract finished\n";
    }
    std::string buff{};
    json.clear();
    json["head"] = "respond";
    json["type"] = "retract";
    json["anwser"] = anwser;
    buff = json.encode();

    memset(sendMsg, 0, sizeof(sendMsg));
    len = strlen(buff.c_str());
    memcpy(sendMsg, &len, sizeof(int));
    memcpy(sendMsg + sizeof(int), buff.c_str(), len);
    send(findRival(connfd), sendMsg, strlen(buff.c_str()) + sizeof(int), 0);

    std::cout << "[Send request Info: " << buff << std::endl;
}

void gobangserver::createRoom(int connfd) { // 创建房间
    if (getRoom(connfd) == lobby.end()) {
        room r;
        r.blackPlayer = players[connfd];
        players[connfd]->status = IN_ROOM;
        players[connfd]->m_isTurn = true;
        players[connfd]->m_color = BLACK_CHESS;
        players[connfd]->m_name = "Black Player";
        players[connfd]->m_prepare = false;
        r.roomID = connfd;
        lobby[connfd] = r;
    } else
        std::cout << "Room has always exist\n";
}

void gobangserver::joinRoom(int connfd, int roomID) { // 加入房间
    if (lobby.find(roomID) != lobby.end()) {          // 房间存在
        if (lobby[roomID].whitePlayer == nullptr) {
            lobby[roomID].whitePlayer = players[connfd]; // 将玩家放入房间
                                                         // 改变玩家信息
            players[connfd]->status = IN_ROOM;
            players[connfd]->m_rivalConnfd =
                getRoom(connfd)->second.blackPlayer->m_connfd;
            getRoom(connfd)->second.blackPlayer->m_rivalConnfd = connfd;
            players[connfd]->m_isTurn = false;
            players[connfd]->m_color = WHITE_CHESS;
            players[connfd]->m_name = "White Player";
            players[connfd]->m_prepare = false;
        }
    }
    std::cout << "[CONNFD: " << connfd << " [Join room: " << roomID
              << std::endl;
}

void gobangserver::quitRoom(int connfd) {
    if (players[connfd]->status == WATCHING &&
        getWatchersRoom(connfd) != lobby.end()) { // 是观战者
        for (auto i = getWatchersRoom(connfd)->second.watchers.begin();
             i != getWatchersRoom(connfd)->second.watchers.end(); i++) {
            if (connfd == (*i)) {
                getWatchersRoom(connfd)->second.watchers.erase(i); // 擦除观战者
                break;
            }
        }
    } else if (players[connfd]->status == IN_ROOM) { // 在房间中，游戏未开始
        if (getChessColor(connfd) == BLACK_CHESS) { // 黑棋退出
            if (findRival(connfd) != -1) {          // 白棋也在
                int whiteid = findRival(connfd);
                std::vector<int> watcher(
                    getRoom(connfd)->second.watchers); // 拷贝构造watchers
                lobby.erase(connfd);
                createRoom(whiteid);
                for (int i = 0; i < watcher.size(); i++) {
                    getRoom(whiteid)->second.watchers.push_back(watcher[i]);
                }
                getRoom(whiteid)->second.blackPlayer->m_rivalConnfd = -1;
            } else { // 白棋不在，只有黑棋
                for (int i = 0; i < getRoom(connfd)->second.watchers.size();
                     i++) {
                    players[getRoom(connfd)->second.watchers[i]]->status =
                        IN_LOBBY;
                }
                lobby.erase(getRoom(connfd)->second.roomID); // 删除房间
            }
        } else if (getChessColor(connfd) == WHITE_CHESS) {
            auto i = getRoom(connfd);
            i->second.whitePlayer->m_rivalConnfd = -1;
            i->second.blackPlayer->m_rivalConnfd = -1;
            i->second.whitePlayer = nullptr;
        }
    } else if (players[connfd]->status == GAMING) {
        sentResultInfo(findRival(connfd),
                       getChessColor(findRival(connfd))); // 向对方发送对局结果
        restart(connfd);
        quitRoom(connfd);
    } else {
        // do nothing
    }
    players[connfd]->status = IN_LOBBY;
}

void gobangserver::quitLobby(int connfd) {
    close(connfd);
    auto iter = players.find(connfd);
    if (iter != players.end()) {
        delete iter->second;
        iter->second = nullptr;
        players.erase(iter++);

        // tcpServer.closeClient();
    }
}

void gobangserver::restart(int connfd) {
    getRoom(connfd)->second.blackPlayer->m_prepare = false;
    getRoom(connfd)->second.whitePlayer->m_prepare = false;
    getRoom(connfd)->second.isGaming = false;

    players[connfd]->status = IN_ROOM;
    players[findRival(connfd)]->status = IN_ROOM;

    getRoom(connfd)->second.m_board.clear();

    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++)
            getRoom(connfd)->second.positionStatus[i][j] = NO_CHESS;
    }

    getRoom(connfd)->second.turn = BLACK_CHESS;
    getRoom(connfd)->second.m_step = 0;
}

void gobangserver::disconnect(int connfd) { // 断开连接
    // 判定输赢
    tcpServer.closeClient();
}

void gobangserver::drop(int connfd, int x, int y) { // 落子
    if (getRoom(connfd) != lobby.end()) {
        if (connfd == getRoom(connfd)->second.blackPlayer->m_connfd) {
            getRoom(connfd)->second.currentChess =
                chess(x, y, BLACK_CHESS,
                      getRoom(connfd)->second.m_step); // 创建棋子
            getRoom(connfd)->second.m_board.push_back(
                getRoom(connfd)->second.currentChess); // 棋子放入容器
            getRoom(connfd)->second.positionStatus[x][y] =
                BLACK_CHESS; // 改变棋盘状态
            turnToNext(connfd);
            if (isWin(connfd, x, y, BLACK_CHESS) == BLACK_CHESS) { // 黑棋赢
                sentResultInfo(connfd, BLACK_CHESS);
                sentResultInfo(findRival(connfd), BLACK_CHESS);
                restart(connfd);
            }
        } else if (connfd == getRoom(connfd)->second.whitePlayer->m_connfd) {
            getRoom(connfd)->second.currentChess =
                chess(x, y, WHITE_CHESS, getRoom(connfd)->second.m_step);
            getRoom(connfd)->second.m_board.push_back(
                getRoom(connfd)->second.currentChess);
            getRoom(connfd)->second.positionStatus[x][y] = WHITE_CHESS;
            turnToNext(connfd);
            if (isWin(connfd, x, y, WHITE_CHESS) == WHITE_CHESS) { // 白棋赢
                sentResultInfo(connfd, WHITE_CHESS);
                sentResultInfo(findRival(connfd), WHITE_CHESS);
                restart(connfd);
            }
        } else
            std::cout << "Error! Player is not exist.\n";
    }
    std::cout << "[CONNFD: " << connfd << "[Drop: x=" << x << " y=" << y
              << std::endl;
}

int gobangserver::isWin(int connfd, int x, int y,
                        int color) { // 输赢判断函数
    for (int i = -4; i <= 0; i++) {
        if (getRoom(connfd)->second.positionStatus[x + i][y] == color &&
            getRoom(connfd)->second.positionStatus[x + i + 1][y] == color &&
            getRoom(connfd)->second.positionStatus[x + i + 2][y] == color &&
            getRoom(connfd)->second.positionStatus[x + i + 3][y] == color &&
            getRoom(connfd)->second.positionStatus[x + i + 4][y] == color) {
            return color;
        }
    }
    for (int i = -4; i <= 0; i++) {
        if (getRoom(connfd)->second.positionStatus[x][y + i] == color &&
            getRoom(connfd)->second.positionStatus[x][y + i + 1] == color &&
            getRoom(connfd)->second.positionStatus[x][y + i + 2] == color &&
            getRoom(connfd)->second.positionStatus[x][y + i + 3] == color &&
            getRoom(connfd)->second.positionStatus[x][y + i + 4] == color) {
            return color;
        }
    }
    for (int i = -4; i <= 0; i++) {
        if (getRoom(connfd)->second.positionStatus[x + i][y + i] == color &&
            getRoom(connfd)->second.positionStatus[x + i + 1][y + i + 1] ==
                color &&
            getRoom(connfd)->second.positionStatus[x + i + 2][y + i + 2] ==
                color &&
            getRoom(connfd)->second.positionStatus[x + i + 3][y + i + 3] ==
                color &&
            getRoom(connfd)->second.positionStatus[x + i + 4][y + i + 4] ==
                color) {
            return color;
        }
    }
    for (int i = -4; i <= 0; i++) {
        if (getRoom(connfd)->second.positionStatus[x - i][y + i] == color &&
            getRoom(connfd)->second.positionStatus[x - i - 1][y + i + 1] ==
                color &&
            getRoom(connfd)->second.positionStatus[x - i - 2][y + i + 2] ==
                color &&
            getRoom(connfd)->second.positionStatus[x - i - 3][y + i + 3] ==
                color &&
            getRoom(connfd)->second.positionStatus[x - i - 4][y + i + 4] ==
                color) {
            return color;
        }
    }
    return NO_CHESS;
}

void gobangserver::prepare(int connfd) { // 准备
    if (players[connfd]->status == IN_ROOM && findRival(connfd) != -1 &&
        players[findRival(connfd)]->status == IN_ROOM) { // 在房间中，且双方都在
        players[connfd]->m_prepare = true; // 将自己设置为准备
    } else
        std::cout << "prepare false!!!\n";
}

void gobangserver::watchMatch(int connfd, int roomID) {
    if (getRoom(roomID) != lobby.end()) {
        players[connfd]->status = WATCHING;
        getRoom(roomID)->second.watchers.push_back(connfd);
    }
}
