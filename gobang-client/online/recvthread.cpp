#include "recvthread.h"

recvthread::recvthread(QObject *parent) : QThread(parent) {}

void recvthread::run() {
    char buff[1024];
    while (true) {
        memset(buff, 0, sizeof(buff));
        int len = 0;
        if (-1 == recv(m_sockfd, &len, sizeof(int), 0)) {
            perror("Recv size of data error\n");
        }

        if (0 == len)
            continue;

        std::cout << "len = " << len << std::endl;

        ssize_t isize = recv(m_sockfd, buff, len, 0);

        std::cout << "buff = " << buff << std::endl;

        if (isize <= 0)
            break;

        std::string buffStr = std::string(buff, isize);
        emit sendBuff(buff);
    }
}

void recvthread::recvSockfd(int sockfd) { m_sockfd = sockfd; }
