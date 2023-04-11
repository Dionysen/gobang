#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QDialog>

namespace Ui {
class SettingDialog;
}

class SettingDialog : public QDialog {
    Q_OBJECT

  public:
    explicit SettingDialog(QWidget *parent = nullptr);
    ~SettingDialog();

  private slots:
    void on_SettingDialog_accepted();

  signals:
    void signalAcceptResult(int diff, int color, QString time);

  private:
    Ui::SettingDialog *ui;
};

#endif // SETTINGDIALOG_H
