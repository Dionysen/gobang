#ifndef RECVTHREAD_H
#define RECVTHREAD_H

#include <QThread>
#include <iostream>
#include <string>

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/unistd.h>
#endif

class recvthread : public QThread {
    Q_OBJECT
  public:
    explicit recvthread(QObject *parent = nullptr);

  protected:
    void run() override;

  public slots:
    void recvSockfd(int sockfd);
  signals:
    void sendBuff(std::string buff);

  private:
    int m_sockfd;
};

#endif // RECVTHREAD_H
