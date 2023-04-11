#ifndef TCPSERVER_H_
#define TCPSERVER_H_

#pragma once
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <unistd.h>
#endif

#define MAXLINK 1024
#define DEFAULT_PORT 16556

class tcpserver {
  private:
    int m_socklen;
    struct sockaddr_in m_servaddr;
    struct sockaddr_in m_clientaddr;

  public:
    int epld = epoll_create(10); // 创建epoll实例
    struct epoll_event ev;       // 创建epoll事件结构体
    int m_sockfd, m_connfd;
    bool m_btimeout;
    tcpserver();
    bool initServer(const unsigned int port);
    void closeListen();
    void closeClient();
    bool accept();
    char *getIP(); // Return IP address of a client
};

#endif /*TCPSERVER*/
