#include "settingdialog.h"
#include "ui_settingdialog.h"
#include <qobjectdefs.h>

SettingDialog::SettingDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::SettingDialog) {
    ui->setupUi(this);
}

SettingDialog::~SettingDialog() { delete ui; }

void SettingDialog::on_SettingDialog_accepted() {
    int difficulty = ui->comboBox->currentIndex();
    int color = ui->comboBox_2->currentIndex();
    QString time = ui->lineEdit->text();
    emit signalAcceptResult(difficulty, color, time);
}
