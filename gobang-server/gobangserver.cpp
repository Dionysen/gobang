#include "gobangserver.h"

gobangserver::gobangserver() { tcpServer.initServer(DEFAULT_PORT); }

int gobangserver::findRival(int connfd) { // Find the socket of the opponent and
                                          // return -1 when there is no opponent
    return players.at(connfd)->m_rivalConnfd;
}

std::map<int, room>::iterator
gobangserver::getRoom(int connfd) { // The first value of the return value is
                                    // the room ID, second is the player, and if
                                    // it is not found, it returns lobby.end()
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

std::map<int, room>::iterator
gobangserver::getWatchersRoom(int connfd) { // Looking for the watcher's room,
                                            // can't find the return lobby.end()
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

void gobangserver::turnToNext(int connfd) {
    if (getRoom(connfd)->second.turn == BLACK_CHESS) {
        getRoom(connfd)->second.turn = WHITE_CHESS;
    } else
        getRoom(connfd)->second.turn = BLACK_CHESS;
}

void gobangserver::updateInfo() { // From player information, update to lobby
                                  // and room information
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
    updateInfo();         // Update game state
    sentLobbyInfo();      // Send lobby messages to everyone
    sentUserInfo();       // Send your own information to all users
    sentRoomInfo(connfd); // If the user is in the room, the room information is
                          // sent to both contestants and all spectators
    sentMatchInfo(connfd); // If the user is in the room, match information is
                           // sent to both players and all spectators
}

void gobangserver::sentLobbyInfo() {
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

    std::cout << "Server: sent lobby info： " << (std::string)sendMsg
              << std::endl;
    for (auto i : players) // Send lobby messages to everyone
    {
        if (-1 == send(i.second->m_connfd, sendMsg,
                       strlen(buff.c_str()) + sizeof(int), 0)) {
            std::cout << "Failed to sent info\n";
        }
    }
}

void gobangserver::sentMatchInfo(int connfd) { // Send checkerboard information

    std::string buff{};
    buff.clear();

    memset(sendMsg, 0, sizeof(sendMsg));

    json.clear();

    if (players[connfd]->status == IN_LOBBY) { // In lobby
        // do nothing
    } else if (players[connfd]->status == IN_ROOM ||
               players[connfd]->status == GAMING) { // is player
        json["head"] = "board";
        json["connfd"] = connfd;
        json["turn"] = getRoom(connfd)->second.turn; // turn
        auto &nodeBoard = json["board"];
        for (int i = 0; i < 15; i++) {
            for (int j = 0; j < 15; j++) {
                auto &node = nodeBoard[i];
                node[std::to_string(i)][j] =
                    getRoom(connfd)->second.positionStatus[i][j];
            }
        }
        // Players may change, so update the board information to everyone who
        // needs it
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
    } else if (players[connfd]->status == WATCHING) { // is watcher
        json["head"] = "board";
        json["connfd"] = connfd;
        json["turn"] = WATCHING; // watcher's turn always is 3
        auto &nodeBoard = json["board"];
        for (int i = 0; i < 15; i++) {
            for (int j = 0; j < 15; j++) {
                auto &node = nodeBoard[i];
                node[std::to_string(i)][j] =
                    getWatchersRoom(connfd)->second.positionStatus[i][j];
            }
        }
        // The audience will not change the state of the board, just return the
        // same way
        buff = json.encode();

        len = strlen(buff.c_str());
        memcpy(sendMsg, &len, sizeof(int));
        memcpy(sendMsg + sizeof(int), buff.c_str(), len);

        send(connfd, sendMsg, strlen(buff.c_str()) + sizeof(int), 0);
    }
    std::cout << "Server: sent board info： " << sendMsg << std::endl;
}

void gobangserver::sentUserInfo() { // Send user information to a single client

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

        std::cout << "Server: sent user info： " << sendMsg << std::endl;
        send(i.first, sendMsg, strlen(buff.c_str()) + sizeof(int), 0);
    }
}

void gobangserver::sentRoomInfo(int connfd) { // Send in-room player information

    std::string buff{};
    buff.clear();

    memset(sendMsg, 0, sizeof(sendMsg));

    json.clear();
    json["head"] = "room";
    json["connfd"] = connfd;
    auto &nodeRoom = json["room"];

    if (players[connfd]->status == WATCHING) { // is watcher
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
             0); // The audience's message does not change the internal display
                 // status of the room, so it only needs to be sent back the
                 // same way

    } else if (players[connfd]->status == IN_ROOM ||
               players[connfd]->status ==
                   GAMING) { // In the room or in the game
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

        buff = json.encode(); // Players may change in-room information, so
                              // update in-room information to each viewer

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

    std::cout << "Server: sent room info: " << sendMsg << std::endl;
}

void gobangserver::sentResultInfo(int connfd,
                                  int color) { // Send match results to connfd

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
        std::cout << "Server: Sent result info " << buff << std::endl;
    }
}

// After receiving the signal from the client, it reacts according to the parsed
// information
void gobangserver::parseInfo(int connfd, char buff[1024]) {

    open::OpenJson json;
    json.decode(buff);

    std::cout << "Client: " << connfd << " Sent info: " << buff << std::endl;

    std::string action = json["action"].s();

    if (action.compare("createRoom") == 0) { // The signal is to create a room
        createRoom(connfd);
    } else if (action.compare("toLobby") ==
               0) { // The signal is to enter the lobby
        players[connfd]->m_connfd = connfd;
    } else if (action.compare("joinRoom") ==
               0) { // The signal is to join the room
        joinRoom(connfd, json["roomID"].i32());
    } else if (action.compare("watchMatch") ==
               0) { // The signal is to watch the game
        watchMatch(connfd, json["roomID"].i32());
    } else if (action.compare("quitRoom") ==
               0) { // The signal is to exit the room
        quitRoom(connfd);
    } else if (action.compare("quitLobby") ==
               0) { // The signal is to exit the lobby
        quitLobby(connfd);
    } else if (action.compare("restart") ==
               0) { // The signal is to start a new round
        restart(connfd);
    } else if (action.compare("prepare") == 0) { // Signals are ready
        prepare(connfd);
    } else if (action.compare("drop") == 0) { // The signal is a drop
        int x{json["x"].i32()};
        int y{json["y"].i32()};
        drop(connfd, x, y);
    } else if (action.compare("request") == 0) {      // The signal is a request
        if (json["type"].s().compare("retract") == 0) // Ask for repentance
            // Send a repentance request
            requestRetract(connfd);
    } else if (action.compare("respond") == 0) { // The signal is a response
        if (json["type"].s().compare("retract") == 0) // Respond to Repentance
            respondRetract(connfd, json["anwser"].b());
    } else if (action.compare("concede") ==
               0) { // The signal is to throw in the towel
        if (getRoom(connfd)->second.whitePlayer->m_connfd == connfd) {
            sentResultInfo(connfd, BLACK_CHESS);
            sentResultInfo(findRival(connfd), BLACK_CHESS);
            restart(connfd);
        } else {
            sentResultInfo(connfd, BLACK_CHESS);
            sentResultInfo(findRival(connfd), BLACK_CHESS);
            restart(connfd);
        }
    } else if (action.compare("exit") ==
               0) { // The signal is exit (considered as a towel throw
    } else {
        std::cout << "Parse error.\n";
    }
    sentInfo(connfd);
}

// action
void gobangserver::retract(int connfd) {
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
void gobangserver::requestRetract(int connfd) {
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

void gobangserver::respondRetract(int connfd, bool anwser) {
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

void gobangserver::createRoom(int connfd) {
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

void gobangserver::joinRoom(int connfd, int roomID) {
    if (lobby.find(roomID) != lobby.end()) { // Room exists
        if (lobby[roomID].whitePlayer == nullptr) {
            lobby[roomID].whitePlayer =
                players[connfd]; // Place the player in the room
                                 // Change player information
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
        getWatchersRoom(connfd) != lobby.end()) { // Be spectators
        for (auto i = getWatchersRoom(connfd)->second.watchers.begin();
             i != getWatchersRoom(connfd)->second.watchers.end(); i++) {
            if (connfd == (*i)) {
                getWatchersRoom(connfd)->second.watchers.erase(
                    i); // Erase spectators
                break;
            }
        }
    } else if (players[connfd]->status ==
               IN_ROOM) { // In the room, the game does not start
        if (getChessColor(connfd) == BLACK_CHESS) { // Black Chess exits
            if (findRival(connfd) != -1) {          // White chess is also there
                int whiteid = findRival(connfd);
                std::vector<int> watcher(
                    getRoom(connfd)
                        ->second.watchers); // Copy construct watchers
                lobby.erase(connfd);
                createRoom(whiteid);
                for (int i = 0; i < watcher.size(); i++) {
                    getRoom(whiteid)->second.watchers.push_back(watcher[i]);
                }
                getRoom(whiteid)->second.blackPlayer->m_rivalConnfd = -1;
            } else { // White chess is not there, only black chess
                for (int i = 0; i < getRoom(connfd)->second.watchers.size();
                     i++) {
                    players[getRoom(connfd)->second.watchers[i]]->status =
                        IN_LOBBY;
                }
                lobby.erase(getRoom(connfd)->second.roomID); // Delete the room
            }
        } else if (getChessColor(connfd) == WHITE_CHESS) {
            auto i = getRoom(connfd);
            i->second.whitePlayer->m_rivalConnfd = -1;
            i->second.blackPlayer->m_rivalConnfd = -1;
            i->second.whitePlayer = nullptr;
        }
    } else if (players[connfd]->status == GAMING) {
        sentResultInfo(findRival(connfd),
                       getChessColor(findRival(
                           connfd))); // Send match results to the other side
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

void gobangserver::disconnect(int connfd) { // Disconnect
    tcpServer.closeClient();
}

void gobangserver::drop(int connfd, int x, int y) {
    if (getRoom(connfd) != lobby.end()) {
        if (connfd == getRoom(connfd)->second.blackPlayer->m_connfd) {
            getRoom(connfd)->second.currentChess =
                chess(x, y, BLACK_CHESS,
                      getRoom(connfd)->second.m_step); // Create chess
            getRoom(connfd)->second.m_board.push_back(
                getRoom(connfd)->second.currentChess); // The pieces are placed
                                                       // in containers
            getRoom(connfd)->second.positionStatus[x][y] =
                BLACK_CHESS; // Change the chessboard status
            turnToNext(connfd);
            if (isWin(connfd, x, y, BLACK_CHESS) ==
                BLACK_CHESS) { // Black chess wins
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
            if (isWin(connfd, x, y, WHITE_CHESS) ==
                WHITE_CHESS) { // White chess wins
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
                        int color) { // Win and loss judgment function
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

void gobangserver::prepare(int connfd) {
    if (players[connfd]->status == IN_ROOM && findRival(connfd) != -1 &&
        players[findRival(connfd)]->status ==
            IN_ROOM) { // In the room, and both sides are there
        players[connfd]->m_prepare = true; // Set self up to prepare
    } else
        std::cout << "prepare false!\n";
}

void gobangserver::watchMatch(int connfd, int roomID) {
    if (getRoom(roomID) != lobby.end()) {
        players[connfd]->status = WATCHING;
        getRoom(roomID)->second.watchers.push_back(connfd);
    }
}
