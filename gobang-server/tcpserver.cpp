#include "tcpserver.h"

tcpserver::tcpserver()
    : m_sockfd(-1), m_connfd(-1), m_socklen(0), m_btimeout(false) {
#ifdef WIN32
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    WSAStartup(sockVersion, &wsaData);
#endif
}

bool tcpserver::initServer(const unsigned int port) {
    if (m_sockfd > 0) {
        close(m_sockfd);
        m_sockfd = -1;
    }

    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == m_sockfd) {
        printf("Create socket error(%d): %s\n", errno, strerror(errno));
        return false;
    } // create socket file discription

    int nagleStatus = 1;
    int result = setsockopt(m_sockfd, IPPROTO_TCP, TCP_NODELAY,
                            (char *)&nagleStatus, sizeof(int));
    if (result == -1) {
        throw "Failed to change nagle status\n";
    }

    bzero(&m_servaddr, sizeof(m_servaddr)); // Zero the socket fd
    m_servaddr.sin_family = AF_INET;        // IPv4
    m_servaddr.sin_addr.s_addr =
        htonl(INADDR_ANY); // Any IP address can connect
    m_servaddr.sin_port =
        htons(DEFAULT_PORT); // Set the port of server address struct
    if (-1 ==
        bind(m_sockfd, (struct sockaddr *)&m_servaddr, sizeof(m_servaddr))) {
        printf("Bind error(%d): %s\n", errno, strerror(errno));
        closeListen();
        return false;
    } // Bind the socket fd to socket address
    if (-1 == listen(m_sockfd, MAXLINK)) {
        printf("Listen error(%d): %s\n", errno, strerror(errno));
        closeListen();
        return false;
    } // listening

    m_socklen = sizeof(struct sockaddr_in);

    if (epld < 0) {
        perror("epoll create error");
        return -1;
    }

    // Put the listening socket into the epoll

    ev.events = EPOLLIN;
    ev.data.fd = m_sockfd;

    if (epoll_ctl(epld, EPOLL_CTL_ADD, m_sockfd, &ev) < 0) {
        perror("epoll_ctl error");
        return -1;
    }

    return true;
}

void tcpserver::closeListen() {
    if (m_sockfd > 0) {
        close(m_sockfd);
        m_sockfd = -1;
    }
}
void tcpserver::closeClient() {
    if (m_connfd > 0) {
        close(m_connfd);
        m_connfd = -1;
    }
}

bool tcpserver::accept() {
    if (-1 == m_sockfd) {
        return false;
    }
    m_connfd = ::accept(m_sockfd, NULL, NULL);
    if (-1 == m_connfd) {
        printf("Accept error(%d): %s\n", errno, strerror(errno));
        return false;
    }
    return true;
}

char *tcpserver::getIP() { return inet_ntoa(m_clientaddr.sin_addr); }
