#include "gobangserver.h"
#include <iostream>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <strings.h>
#include <pthread.h>
#endif

#define MAX_CONN 1024
gobangserver gobangServer;

void stopServerRunning(int p);
void threadHandle(int connfd);

int main()
{
#ifdef _WIN32
    signal(SIGINT, stopServerRunning);
    signal(SIGTERM, stopServerRunning);

    while (true)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        int maxfd = gobangServer.tcpServer.m_sockfd;

        // Add server socket to set
        FD_SET(gobangServer.tcpServer.m_sockfd, &readfds);

        for (const auto& player : gobangServer.players)
        {
            FD_SET(player.first, &readfds);
            if (player.first > maxfd)
            {
                maxfd = player.first;
            }
        }

        // Wait for activity on any socket
        int activity = select(maxfd + 1, &readfds, nullptr, nullptr, nullptr);
        if (activity < 0)
        {
            std::cerr << "select error: " << WSAGetLastError() << std::endl;
            break;
        }

        // Check if there's a new connection
        if (FD_ISSET(gobangServer.tcpServer.m_sockfd, &readfds))
        {
            struct sockaddr_in client_addr;
            int                client_addr_len = sizeof(client_addr);

            int client_sockfd = accept(gobangServer.tcpServer.m_sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
            if (client_sockfd == INVALID_SOCKET)
            {
                std::cerr << "accept error: " << WSAGetLastError() << std::endl;
                continue;
            }

            std::cout << inet_ntoa(client_addr.sin_addr) << " is connecting\n";

            // Save information for the client
            gobangServer.players[client_sockfd] = new player(client_sockfd);
        }

        // Check for messages from clients
        for (const auto& player : gobangServer.players)
        {
            int fd = player.first;

            if (FD_ISSET(fd, &readfds))
            {
                char   buff[1024];
                size_t isize = recv(fd, buff, sizeof(buff), 0);
                if (isize <= 0)
                {
                    std::cout << "Client " << fd << " disconnected\n";
                    closesocket(fd);
                    gobangServer.players.erase(fd);
                }
                else
                {
                    buff[isize] = '\0';  // Null-terminate the received data
                    std::cout << "recv:(size = " << isize << ") " << buff << std::endl;
                    gobangServer.parseInfo(fd, buff);
                }
            }
        }
    }

    // Cleanup
    closesocket(gobangServer.tcpServer.m_sockfd);
    WSACleanup();

#else
    signal(SIGINT, stopServerRunning);
    signal(15, stopServerRunning);
    while (true)
    {


        struct epoll_event evs[MAX_CONN];
        int                maxfd = epoll_wait(gobangServer.tcpServer.epld, evs, MAX_CONN, -1);
        if (maxfd < 0)
        {
            std::cout << "epoll_wait error\n";
            break;
        }
        for (int i = 0; i < maxfd; i++)
        {
            int fd = evs[i].data.fd;
            // If the fd that is listening receives the message, it means that
            // there is a client connection
            if (fd == gobangServer.tcpServer.m_sockfd)
            {
                struct sockaddr_in client_addr;
                socklen_t          client_addr_len = sizeof(client_addr);

                int client_sockfd = accept(gobangServer.tcpServer.m_sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
                if (client_sockfd < 0)
                {
                    std::cout << "accept error\n";
                    continue;
                }

                // Add the client's socket to the epoll
                struct epoll_event ev_client;
                ev_client.events  = EPOLLIN;
                ev_client.data.fd = client_sockfd;
                if (epoll_ctl(gobangServer.tcpServer.epld, EPOLL_CTL_ADD, client_sockfd, &ev_client) < 0)
                {
                    std::cout << "epoll_ctl error\n";
                    break;
                }
                std::cout << client_addr.sin_addr.s_addr << " is connecting\n";

                // Saves information for the client
                gobangServer.players[client_sockfd] = new player(client_sockfd);
            }
            else
            {  // If a message comes from the client
                char    buff[1024];
                ssize_t isize = recv(fd, buff, 1024, 0);
                std::cout << "revc:(size = " << isize << ") " << buff << std::endl;
                if (isize < 0)
                {  // Less than zero, exception
                    break;
                }
                else if (isize == 0)
                {  // Equal to zero indicates disconnection
                    std::cout << "Client " << fd << " disconnect\n";
                    close(fd);
                    epoll_ctl(gobangServer.tcpServer.epld, EPOLL_CTL_DEL, fd, 0);
                    auto iter = gobangServer.players.find(fd);
                    if (iter != gobangServer.players.end())
                    {
                        delete iter->second;
                        iter->second = nullptr;
                        gobangServer.players.erase(iter++);
                    }
                }
                else
                {
                    gobangServer.parseInfo(fd, buff);
                }
            }
        }
    }
    close(gobangServer.tcpServer.epld);
    close(gobangServer.tcpServer.m_sockfd);
#endif
    return 0;
}

void stopServerRunning(int p)
{  // Use ctrl + c to stop server
    gobangServer.tcpServer.closeClient();
    std::cout << "\nStop Server ..." << std::endl;
    exit(0);
}
