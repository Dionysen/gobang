#ifndef ROBOTTHREAD_H
#define ROBOTTHREAD_H

#include <QThread>
#include <iostream>
#include <qpoint.h>

class robotthread : public QThread {
    Q_OBJECT
  public:
    explicit robotthread(QObject *parent = nullptr);

  protected:
    void run() override;

  public slots:
    void isTurn(int x, int y);
  signals:
    void signalSendPosition(int x, int y);

  private:
    QPoint currentPos;
};

#endif // ROBOTTHREAD_H
