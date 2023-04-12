#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "game.h"
#include "home.h"
#include "lobby.h"
#include "onlinegame.h"
#include "recvthread.h"
#include "tcpclient.h"
// #include "threadpool.h"

#include <QMainWindow>
#include <QMessageBox>

#ifdef WIN32
#include <windows.h>
#include <winsock2.h>
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    game *Game = new game(this);
    home *Home = new home(this);
    lobby *Lobby = new lobby(this);
    onlinegame *OnlineGame = new onlinegame(this);
    tcpclient tcpClient;

    // void static parseThreadHandle(onlinegame &OnlineGame, lobby &Lobby,
    //                               int connfd, tcpclient &tcpClient,
    //                               threadpool &pool);

  protected:
    void closeEvent(QCloseEvent *event);

  private slots:

    void backToHome();
    void backToHomeNoAsk();
    void toGamePage();
    void exit();

    void createRoom();
    void joinRoom();
    void dropChess();
  signals:
    void sendConnfd(int connfd);

  private:
    Ui::MainWindow *ui;
    recvthread *recvThread = new recvthread(this);
    // threadpool pool;
};
#endif // MAINWINDOW_H
