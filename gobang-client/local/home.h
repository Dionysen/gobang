#ifndef HOME_H
#define HOME_H

#include <QWidget>

namespace Ui {
class home;
}

class home : public QWidget {
    Q_OBJECT

  public:
    explicit home(QWidget *parent = nullptr);
    ~home();

  private:
    Ui::home *ui;
  signals:
    void signalNewGame();
    void signalConnectToServer();
    void signalHelp();
    void signalExit();

  public slots:
    void newGame();
    void connectToServer();
    void help();
    void exit();
};

#endif // HOME_H
