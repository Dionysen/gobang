#include "tcpclient.h"
#include "../lib/openjson/openjson.h"
#include "lobby.h"
#include "onlinegame.h"
#include <iostream>

tcpclient::tcpclient() : sockfd(0) {}
bool tcpclient::connectToServer() // 连接到服务器
{

#ifdef WIN32
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    WSAStartup(sockVersion, &wsaData);
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
} // 关闭套接字
void tcpclient::disconnect() // 断开连接
{
    if (sockfd > 0) {
        shutdown(sockfd, 2);
        this->closesocket();
        std::cout << "Disconnected\n";
    }
}

void tcpclient::parseInformation(
    onlinegame &OnlineGame, lobby &Lobby,
    std::string
        buff) // 从服务器接受信息，并且解析信息，根据解析出的内容做出响应
{

    open::OpenJson json;
    json.decode(buff);
    // 区分信息类别，做出信息更新

    std::string head = json["head"].s();

    if (head.compare("lobby") == 0) { // 接收到更新大厅的信息
        std::cout << "客户端："
                  << " 收到大厅信息: " << buff << std::endl;
        int numOfRooms = json["nums"].i32();
        auto &nodeLobby = json["lobby"];
        int id = -1, nums = 0, watchers = 0;
        bool isGaming = false;
        Lobby.rooms.clear();
        for (int i = 0; i < numOfRooms; i++) { // 读取大厅信息
            id = nodeLobby[i][0].i32();
            nums = nodeLobby[i][1].i32();
            watchers = nodeLobby[i][2].i32();
            isGaming = nodeLobby[i][3].b();
            Lobby.rooms.push_back(room(id, nums, watchers, isGaming));
        }
        Lobby.updateTable(); // 更新大厅显示

    } else if (head.compare("board") == 0) { // 接收到更新棋盘的信息
        std::cout << "客户端：" << json["connfd"].i32()
                  << " 收到棋盘信息: " << buff << std::endl;
        int positionStatus[15][15]{};
        auto &nodeBoard = json["board"];
        if (json["turn"].i32() == WATCHING) {
            OnlineGame.selfTurn = false;
            OnlineGame.status = WATCHING;
            OnlineGame.siganlUpdateUserInfo();
        } else {
            if (OnlineGame.selfColor == BLACK_CHESS) { // 如果当前是黑棋
                if (json["turn"].i32() == BLACK_CHESS) // 轮到黑棋
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
        for (int i = 0; i < 15; i++) { // 读取棋盘信息
            for (int j = 0; j < 15; j++) {
                auto &node = nodeBoard[i];
                positionStatus[i][j] = node[std::to_string(i)][j].i32();
            }
        }
        OnlineGame.updateBoard(positionStatus); // 更新棋盘显示
        std::cout << "已读取并更新棋盘信息!!!!!!\n";

    } else if (head.compare("room") == 0) { // 更新房间信息
        std::cout << "客户端：" << json["connfd"].i32()
                  << " 收到房间信息: " << buff << std::endl;
        auto &nodeRoom = json["room"];
        OnlineGame.setBlackId(nodeRoom["blackID"].i32());
        OnlineGame.setWhiteId(nodeRoom["whiteID"].i32());
        OnlineGame.setBlackName(nodeRoom["blackName"].s());
        OnlineGame.setWhiteName(nodeRoom["whiteName"].s());
        OnlineGame.updateRoomInfo();

    } else if (head.compare("result") == 0) { // 更新输赢信息
        if (json["winner"].i32() == BLACK_CHESS) {
            OnlineGame.respondWin(BLACK_CHESS);
        } else {
            OnlineGame.respondWin(WHITE_CHESS);
        }

    } else if (head.compare("request") == 0) {
        // 对方请求悔棋，这里进行问答回应
        OnlineGame.respondRetract();

    } else if (head.compare("respond") == 0) {
        // 解析出对方是否同意悔棋
    } else if (head.compare("user") == 0) { // 更新玩家信息，决定所在界面和状态
        std::cout << "客户端：" << json["connfd"].i32()
                  << " 收到玩家信息: " << buff << std::endl;
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
    }
}

bool tcpclient::joinLobby() // 连接到服务器，发送信号joinLobby
{
    std::cout << "-----------------------------------------\n";
    if (connectToServer()) {
        open::OpenJson json;
        json["action"] = "toLobby";
        std::string buff = json.encode();
        send(sockfd, buff.data(), buff.size(), 0);
        // std::cout << "客户端 " << sockfd << " 发送信息： " << buff <<
        // std::endl;
        return true;
    } else {
        return false;
    }
}
void tcpclient::createRoom() // 发送信号createRoom
{
    std::cout << "-----------------------------------------\n";
    open::OpenJson json;
    json["action"] = "createRoom";
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
    // std::cout << "客户端 " << sockfd << " 发送信息： " << buff <<
    // std::endl;
} // 创建房间

void tcpclient::joinRoom(int roomID) // 发送信号joinRoom
{
    std::cout << "-----------------------------------------\n";
    open::OpenJson json;
    json["action"] = "joinRoom";
    json["roomID"] = roomID;
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
    // std::cout << "客户端 " << sockfd << " 发送信息： " << buff <<
    // std::endl;
} // 加入房间

void tcpclient::watchMatch(int roomID) // 观看比赛
{
    std::cout << "-----------------------------------------\n";
    open::OpenJson json;
    json["action"] = "watchMatch";
    json["roomID"] = roomID;
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
    // std::cout << "客户端 " << sockfd << " 发送信息： " << buff <<
    // std::endl;
}

void tcpclient::prepared() {
    std::cout << "-----------------------------------------\n";
    open::OpenJson json;
    json["action"] = "prepare";
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
    // std::cout << "客户端 " << sockfd << " 发送信息： " << buff <<
    // std::endl;
} // 准备
void tcpclient::drop(int x, int y) // 发送落子位置
{
    std::cout << "-----------------------------------------\n";
    open::OpenJson json;
    json["action"] = "drop";
    json["x"] = x;
    json["y"] = y;
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
    // std::cout << "客户端 " << sockfd << " 发送信息： " << buff <<
    // std::endl;
} // 落子

void tcpclient::quitLobby() // 退出大厅，断开连接
{
    std::cout << "-----------------------------------------\n";
    json.clear();
    json["action"] = "quitLobby";
    std::string buff = json.encode();
    // send(sockfd, buff.data(), buff.size(), 0);
    // std::cout << "客户端 " << sockfd << " 发送信息： " << buff <<
    // std::endl;
    disconnect();
}
void tcpclient::quitRoom() // 退出房间，回到大厅
{
    std::cout << "-----------------------------------------\n";
    json.clear();
    json["action"] = "quitRoom";
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
    // std::cout << "客户端 " << sockfd << " 发送信息： " << buff <<
    // std::endl;
}
void tcpclient::requestRetract() // 请求悔棋
{
    std::cout << "-----------------------------------------\n";
    json.clear();
    json["action"] = "request";
    json["type"] = "retract";
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
    // std::cout << "客户端 " << sockfd << " 发送信息： " << buff <<
    // std::endl;
}

void tcpclient::repondRetract(bool anwser) // 回应悔棋请求
{
    std::cout << "-----------------------------------------\n";
    json.clear();
    json["action"] = "respond";
    json["type"] = "retract";
    json["anwser"] = anwser;
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
    // std::cout << "客户端 " << sockfd << " 发送信息： " << buff <<
    // std::endl;
}
void tcpclient::exit() {} // 退出
void tcpclient::concede() {
    std::cout << "-----------------------------------------\n"; // 认输
    json.clear();
    json["action"] = "concede";
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
    // std::cout << "客户端 " << sockfd << " 发送信息： " << buff <<
    // std::endl;
} // 认输

void tcpclient::restartGame() { // 新开一局
    std::cout << "-----------------------------------------\n";
    json.clear();
    json["action"] = "restart";
    std::string buff = json.encode();
    send(sockfd, buff.data(), buff.size(), 0);
    // std::cout << "客户端 " << sockfd << " 发送信息： " << buff <<
    // std::endl;
}
