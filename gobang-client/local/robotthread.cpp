#include "robotthread.h"
#include "ChessEngine.h"
#include <cstdio>
#include <iostream>
#include <ostream>
#include <qobjectdefs.h>
#include <qpoint.h>

robotthread::robotthread(QObject *parent)
    : QThread(parent), currentPos(QPoint(-1, -1)) {}

void robotthread::run() { // robot drop and get the position
    ChessEngine::nextStep(currentPos.x(), currentPos.y());
    emit signalSendPosition(ChessEngine::getLastPosition().x,
                            ChessEngine::getLastPosition().y);
}

void robotthread::isTurn(int x, int y) {
    currentPos.setX(x);
    currentPos.setY(y);
}