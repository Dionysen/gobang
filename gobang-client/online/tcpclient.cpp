#include "tcpclient.h"
#include "../lib/openjson/openjson.h"
#include "lobby.h"
#include "onlinegame.h"

tcpclient::tcpclient() : sockfd(0) {}
bool tcpclient::connectToServer() {

#ifdef WIN32
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if(WSAStartup(sockVersion, &wsaData) != 0){
        return 1;
    }
#endif

    if (0 >= (sockfd = socket(AF_INET, SOCK_STREAM, 0)))
        return false;
    memset(&servaddr, 0, sizeof(sockaddr));
    servaddr.sin_family = AF_INET;

#ifdef WIN32
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);
#else
    inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr);
#endif

    servaddr.sin_port = htons(SERVER_PORT);

    if (-1 == connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) {
        printf("Connect error(%d): %s\n", errno, strerror(errno));
        return false;
    }
    return true;
}

void tcpclient::closesocket() {
#ifdef WIN32
    ::closesocket(sockfd);
#else
    close(sockfd);
#endif
} // Close socket

void tcpclient::disconnect() // disconnect
{
    if (sockfd > 0) {
        shutdown(sockfd, 2);
        this->closesocket();
        std::cout << "Disconnected\n";
    }
}

void tcpclient::parseInformation(onlinegame &OnlineGame, lobby &Lobby,
                                 std::string buff) {

    open::OpenJson json;
    json.decode(buff); // parse string to json

    std::string head = json["head"].s();

    if (head.compare("lobby") == 0) { // Update lobby
        std::cout << "Client: "
                  << " Update Lobby: " << buff << std::endl;
        int numOfRooms = json["nums"].i32();
        auto &nodeLobby = json["lobby"];
        int id = -1, nums = 0, watchers = 0;
        bool isGaming = false;
        Lobby.rooms.clear();
        for (int i = 0; i < numOfRooms; i++) { // Read lobby info
            id = nodeLobby[i][0].i32();
            nums = nodeLobby[i][1].i32();
            watchers = nodeLobby[i][2].i32();
            isGaming = nodeLobby[i][3].b();
            Lobby.rooms.push_back(room(id, nums, watchers, isGaming));
        }
        Lobby.updateTable(); // update display

    } else if (head.compare("board") == 0) { // Update board
        std::cout << "Client: " << json["connfd"].i32()
                  << " Update board: " << buff << std::endl;
        int positionStatus[15][15]{};
        auto &nodeBoard = json["board"];
        if (json["turn"].i32() == WATCHING) {
            OnlineGame.selfTurn = false;
            OnlineGame.status = WATCHING;
            OnlineGame.siganlUpdateUserInfo();
        } else {
            if (OnlineGame.selfColor == BLACK_CHESS) { // if self is black
                if (json["turn"].i32() == BLACK_CHESS) // if turn to black
                    OnlineGame.selfTurn = true;
                else
                    OnlineGame.selfTurn = false;
            } else {
                if (json["turn"].i32() == BLACK_CHESS)
                    OnlineGame.selfTurn = false;
                else
                    OnlineGame.selfTurn = true;
            }
        }
        for (int i = 0; i < 15; i++) { // read board info
            for (int j = 0; j < 15; j++) {
                auto &node = nodeBoard[i];
                positionStatus[i][j] = node[std::to_string(i)][j].i32();
            }
        }
        OnlineGame.updateBoard(positionStatus); // update board display
    } else if (head.compare("room") == 0) {     // Update room info
        std::cout << "Client: " << json["connfd"].i32()
                  << " Read room info: " << buff << std::endl;
        auto &nodeRoom = json["room"];
        OnlineGame.setBlackId(nodeRoom["blackID"].i32());
        OnlineGame.setWhiteId(nodeRoom["whiteID"].i32());
        OnlineGame.setBlackName(nodeRoom["blackName"].s());
        OnlineGame.setWhiteName(nodeRoom["whiteName"].s());
        OnlineGame.updateRoomInfo();

    } else if (head.compare("result") == 0) { // Update result info
        if (json["winner"].i32() == BLACK_CHESS) {
            OnlineGame.respondWin(BLACK_CHESS);
        } else {
            OnlineGame.respondWin(WHITE_CHESS);
        }

    } else if (head.compare("request") == 0) {
        // Rival request
        if (json["type"].s().compare("retract") == 0)
            OnlineGame.respondRetract();
        else if (json["type"].s().compare("replay") == 0)
            OnlineGame.respondReplay();
    } else if (head.compare("respond") == 0) {
        if (json["type"].s().compare("retract") == 0) {
        }
        // nothing
    } else if (head.compare("user") == 0) { // Update User Info
        std::cout << "Client：" << json["connfd"].i32()
                  << " Read user info: " << buff << std::endl;
        switch (json["status"].i32()) {
        case IN_LOBBY:
            OnlineGame.status = IN_LOBBY;
            break;
        case IN_ROOM:
            OnlineGame.status = IN_ROOM;
            break;
        case GAMING:
            OnlineGame.status = GAMING;
            OnlineGame.selfColor = json["selfColor"].i32();
            OnlineGame.selfTurn = json["selfTurn"].b();
            break;
        case WATCHING:
            OnlineGame.status = WATCHING;
            break;
        default:
            break;
        }
        emit OnlineGame.siganlUpdateUserInfo();
        OnlineGame.updateRoomInfo();
    } else if (head.compare("currentChess") == 0) {
        OnlineGame.setCurrentChess(chess(json["x"].i32(), json["y"].i32(),
                                         json["color"].i32(),
                                         json["step"].i32()));
        OnlineGame.update();
    } else if (head.compare("restart") == 0) {
        OnlineGame.newGame();
    }
}

bool tcpclient::joinLobby() // Connect to server and join lobby
{
    if (connectToServer()) {
        open::OpenJson json;
        json["action"] = "toLobby";
        std::string buff = json.encode();
        send(sockfd, buff.data(), buff.size(), 0);
        return true;
    } else {
        return false;
    }
}

void tcpclient::createRoom() // create a Room
{
    open::OpenJson json;
    json["action"] = "createRoom";
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
}

void tcpclient::joinRoom(int roomID) // join a Room
{
    open::OpenJson json;
    json["action"] = "joinRoom";
    json["roomID"] = roomID;
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
}

void tcpclient::watchMatch(int roomID) // Watch a match
{
    open::OpenJson json;
    json["action"] = "watchMatch";
    json["roomID"] = roomID;
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
}

void tcpclient::prepared() {
    open::OpenJson json;
    json["action"] = "prepare";
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
}

void tcpclient::drop(int x, int y) // Send chess position
{
    open::OpenJson json;
    json["action"] = "drop";
    json["x"] = x;
    json["y"] = y;
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
}

void tcpclient::quitLobby() // quit lobby
{
    json.clear();
    json["action"] = "quitLobby";
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
    disconnect();
}

void tcpclient::quitRoom() {
    json.clear();
    json["action"] = "quitRoom";
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
}

void tcpclient::requestRetract() {
    json.clear();
    json["action"] = "request";
    json["type"] = "retract";
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
}

void tcpclient::repondRetract(bool anwser) {
    json.clear();
    json["action"] = "respond";
    json["type"] = "retract";
    json["anwser"] = anwser;
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
}

void tcpclient::requestReplay() {
    json.clear();
    json["action"] = "request";
    json["type"] = "replay";
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
}

void tcpclient::repondReplay(bool anwser) {
    json.clear();
    json["action"] = "respond";
    json["type"] = "replay";
    json["anwser"] = anwser;
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
}

void tcpclient::exit() {}

void tcpclient::concede() {
    json.clear();
    json["action"] = "concede";
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
}

void tcpclient::restartGame() {
    json.clear();
    json["action"] = "restart";
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
}

void tcpclient::setPlayerName(QString name) {
    json.clear();
    json["action"] = "name";
    json["name"] = name.toStdString();
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
}
