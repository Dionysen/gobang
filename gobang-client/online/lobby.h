#ifndef LOBBY_H
#define LOBBY_H
 
#include <QWidget>
#include <iostream>
#include <vector>

namespace Ui {
class lobby;
}
struct room {
    int roomID, numOfPlayers, watchers;
    bool isGaming;
    room(int id, int nums, int watchers, bool isGaming)
        : roomID(id), numOfPlayers(nums), watchers(watchers),
          isGaming(isGaming) {}
};
class lobby : public QWidget {
    Q_OBJECT

  public:
    explicit lobby(QWidget *parent = nullptr);
    ~lobby();

    std::vector<room> rooms;
    void updateTable();

    int getRoomID();
    int getRoomNumOfPlayers();

  private slots:
    void createRoom();
    void joinRoom();
    void backToHome();

  private:
    Ui::lobby *ui;

  signals:
    void signalCreateRoom();
    void siganlJoinRoom();
    void signalBackToHome();
    void signalBackToHomeNoAsk();
    void signalWatchMatch();
};

#endif // LOBBY_H
