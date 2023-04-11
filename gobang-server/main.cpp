#include "gobangserver.h"
#include <cstring>
#include <iostream>
#include <ostream>
#include <pthread.h>
#include <signal.h>
#include <string>
#include <strings.h>
#include <unistd.h>
#include <vector>

#define MAX_CONN 1024
gobangserver gobangServer;

void stopServerRunning(int p);
void threadHandle(int connfd);

int main() {
    signal(SIGINT, stopServerRunning);
    signal(15, stopServerRunning);
    while (true) {
        struct epoll_event evs[MAX_CONN];
        int maxfd = epoll_wait(gobangServer.tcpServer.epld, evs, MAX_CONN, -1);
        if (maxfd < 0) {
            std::cout << "epoll_wait error\n";
            break;
        }
        for (int i = 0; i < maxfd; i++) {
            int fd = evs[i].data.fd;
            // 如果是监听的fd受到消息，则表示有客户端连接
            if (fd == gobangServer.tcpServer.m_sockfd) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);

                int client_sockfd =
                    accept(gobangServer.tcpServer.m_sockfd,
                           (struct sockaddr *)&client_addr, &client_addr_len);
                if (client_sockfd < 0) {
                    std::cout << "accept error\n";
                    continue;
                }

                // 将客户端的socket加入epoll
                struct epoll_event ev_client;
                ev_client.events = EPOLLIN;
                ev_client.data.fd = client_sockfd;
                if (epoll_ctl(gobangServer.tcpServer.epld, EPOLL_CTL_ADD,
                              client_sockfd, &ev_client) < 0) {
                    std::cout << "epoll_ctl error\n";
                    break;
                }
                std::cout << client_addr.sin_addr.s_addr << " is connecting\n";

                // 保存客户端的信息
                gobangServer.players[client_sockfd] = new player(client_sockfd);
            } else { // 如果客户端传来消息
                char buff[1024];
                ssize_t isize = recv(fd, buff, 1024, 0);
                std::cout << "revc:(size = " << isize << ") " << buff
                          << std::endl;
                if (isize < 0) { // 小于零，异常
                    break;
                } else if (isize == 0) { // 等于零说明断开连接
                    std::cout << "Client " << fd << " disconnect\n";
                    close(fd);
                    epoll_ctl(gobangServer.tcpServer.epld, EPOLL_CTL_DEL, fd,
                              0);
                    auto iter = gobangServer.players.find(fd);
                    if (iter != gobangServer.players.end()) {
                        delete iter->second;
                        iter->second = nullptr;
                        gobangServer.players.erase(iter++);
                    }
                } else {
                    gobangServer.parseInfo(fd, buff);
                }
            }
        }
    }
    close(gobangServer.tcpServer.epld);
    close(gobangServer.tcpServer.m_sockfd);

    return 0;
}

void stopServerRunning(int p) { // 使用ctrl + c 退出服务
    gobangServer.tcpServer.closeClient();
    std::cout << "\nStop Server ..." << std::endl;
    exit(0);
}
