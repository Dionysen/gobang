#include "home.h"
#include "ui_home.h"

home::home(QWidget *parent) : QWidget(parent), ui(new Ui::home) {
    ui->setupUi(this);

    connect(ui->pushButton_newGame, SIGNAL(clicked(bool)), this,
            SLOT(newGame()));
    connect(ui->pushButton_PVP, SIGNAL(clicked(bool)), this,
            SLOT(connectToServer()));
    // connect(ui->pushButton_help, SIGNAL(clicked(bool)), this, SLOT(help()));
    connect(ui->pushButton_exit, SIGNAL(clicked(bool)), this, SLOT(exit()));
}

home::~home() { delete ui; }
void home::newGame() { emit signalNewGame(); }
void home::connectToServer() { emit signalConnectToServer(); }
void home::help() { emit signalHelp(); }
void home::exit() { emit signalExit(); }