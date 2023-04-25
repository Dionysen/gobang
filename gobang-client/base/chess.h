#ifndef CHESS_H
#define CHESS_H

#include <QPoint>
class chess {
  private:
    QPoint pos;
    int color; // black:0 white:1 other: 2 ~ *
    int step;

  public:
    chess();
    inline int x() { return pos.x(); }
    inline const int y() { return pos.y(); }
    inline const int getColor() { return color; }
    inline const int getStep() { return step; }
    inline const QPoint getPoint() { return pos; }
    void setX(int x);
    void setY(int y);
    void setColor(int color);
    void setStep(int step);
    void setPoint(QPoint pos);
    chess(int x, int y, int color, int step);
};

#endif // CHESS_H
