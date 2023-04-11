#include "lobby.h"
#include "ui_lobby.h"
#include <QTableWidgetItem>
#include <iostream>
#include <ostream>
#include <qabstractitemview.h>
#include <qheaderview.h>
#include <qitemselectionmodel.h>
#include <qlabel.h>
#include <qlist.h>
#include <qnamespace.h>
#include <qobjectdefs.h>
#include <qpushbutton.h>
#include <qstandarditemmodel.h>
#include <qstringliteral.h>
#include <qtablewidget.h>
#include <unistd.h>
lobby::lobby(QWidget *parent) : QWidget(parent), ui(new Ui::lobby) {
    ui->setupUi(this);

    // 表格显示大厅中的房间
    ui->roomInfo->setColumnCount(4);      // 设置列数为4
    ui->roomInfo->setColumnWidth(0, 100); // 设置列宽
    ui->roomInfo->setColumnWidth(1, 100);
    ui->roomInfo->setColumnWidth(2, 100);
    ui->roomInfo->setColumnWidth(3, 200);
    ui->roomInfo->verticalHeader()->setVisible(false); // 隐藏列表头
    ui->roomInfo->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Fixed); // 设置表头宽度不可改变

    ui->roomInfo->setSelectionBehavior(
        QTableWidget::SelectRows); // 设置只能选择一行
    ui->roomInfo->setSelectionMode(
        QTableWidget::SingleSelection); // 设置只能选中一行
    ui->roomInfo->setEditTriggers(QTableWidget::NoEditTriggers);
    updateTable();

    // 连接按钮与槽函数，其中槽函数会向mainwindow发送信号
    connect(ui->ptn_backToHome, SIGNAL(clicked()), this, SLOT(backToHome()));
    connect(ui->ptn_createRoom, SIGNAL(clicked()), this, SLOT(createRoom()));
    connect(ui->ptn_joinRoom, SIGNAL(clicked()), this, SLOT(joinRoom()));
    connect(ui->ptn_watch, &QPushButton::clicked, this,
            [=] { emit signalWatchMatch(); });
}

lobby::~lobby() { delete ui; }

void lobby::backToHome() { emit signalBackToHomeNoAsk(); }
void lobby::createRoom() { emit signalCreateRoom(); }
void lobby::joinRoom() { emit siganlJoinRoom(); }

int lobby::getRoomID() {
    QList<QTableWidgetItem *> item = ui->roomInfo->selectedItems();
    if (!item.empty()) { // 如果选中，返回选中的房间id
        return item.at(0)->text().toInt();
    } else { // 否则返回-1
        return -1;
    }
}

int lobby::getRoomNumOfPlayers() {
    QList<QTableWidgetItem *> item = ui->roomInfo->selectedItems();
    if (!item.empty()) { // 如果选中，返回选中的房间id
        return item.at(1)->text().toInt();
    } else { // 否则返回-1
        return -1;
    }
}

void lobby::updateTable() {
    ui->roomInfo->setHorizontalHeaderLabels(
        QStringList() << "Room ID"
                      << "Players"
                      << "Watchers"
                      << "Game status"); // 使用匿名对象qstringlist设置表头信息

    ui->roomInfo->setRowCount(rooms.size());

    for (int i = 0; i < rooms.size(); i++) {
        std::cout << "rooms:" << rooms[i].roomID << rooms[i].numOfPlayers
                  << rooms[i].isGaming << rooms[i].watchers << std::endl;
        ui->roomInfo->setItem(
            i, 0, new QTableWidgetItem(QString::number(rooms[i].roomID)));
        ui->roomInfo->setItem(
            i, 1, new QTableWidgetItem(QString::number(rooms[i].numOfPlayers)));
        ui->roomInfo->setItem(
            i, 2, new QTableWidgetItem(QString::number(rooms[i].watchers)));
        if (rooms[i].isGaming)
            ui->roomInfo->setItem(i, 3,
                                  new QTableWidgetItem(QString("Gaming")));
        else
            ui->roomInfo->setItem(i, 3,
                                  new QTableWidgetItem(QString("Wating")));
    }
    for (int i = 0; i < rooms.size(); i++) {
        for (int j = 0; j < 4; j++)
            ui->roomInfo->item(i, j)->setTextAlignment(Qt::AlignHCenter);
    }
}
