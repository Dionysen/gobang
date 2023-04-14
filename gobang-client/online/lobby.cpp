#include "lobby.h"
#include "ui_lobby.h"

lobby::lobby(QWidget *parent) : QWidget(parent), ui(new Ui::lobby) {
    ui->setupUi(this);

    // table shows rooms in lobby
    ui->roomInfo->setColumnCount(4);      // column count is 4
    ui->roomInfo->setColumnWidth(0, 100); // column width
    ui->roomInfo->setColumnWidth(1, 100);
    ui->roomInfo->setColumnWidth(2, 100);
    ui->roomInfo->setColumnWidth(3, 200);
    ui->roomInfo->verticalHeader()->setVisible(false); // Hide table head
    ui->roomInfo->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Fixed); // Fix the table head width

    ui->roomInfo->setSelectionBehavior(
        QTableWidget::SelectRows); // set only select a row
    ui->roomInfo->setSelectionMode(
        QTableWidget::SingleSelection); // set only select one row
    ui->roomInfo->setEditTriggers(QTableWidget::NoEditTriggers);
    updateTable();

    // connect to mainwindow
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
    if (!item.empty()) { // select a item, return room id
        return item.at(0)->text().toInt();
    } else { // or return -1
        return -1;
    }
}

int lobby::getRoomNumOfPlayers() {
    QList<QTableWidgetItem *> item = ui->roomInfo->selectedItems();
    if (!item.empty()) { // select a item, return number of players
        return item.at(1)->text().toInt();
    } else { // or return -1
        return -1;
    }
}

void lobby::updateTable() {
    ui->roomInfo->setHorizontalHeaderLabels(
        QStringList() << "Room ID"
                      << "Players"
                      << "Watchers"
                      << "Game status"); // use QStringList setting table head

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
