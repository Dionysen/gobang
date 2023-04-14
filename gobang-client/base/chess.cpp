#include "chess.h"
#include <qpoint.h>

chess::chess(int x, int y, int color, int step)
    : pos(QPoint(x, y))
    , color(color)
    , step(step)
{
}

chess::chess() { }
void chess::setX(int x) { this->pos.setX(x); }
void chess::setY(int y) { this->pos.setY(y); }
void chess::setColor(int color) { this->color = color; }
void chess::setStep(int step) { this->step = step; }
void chess::setPoint(QPoint pos) { this->pos = pos; }
