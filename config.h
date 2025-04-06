#ifndef CONFIG_H
#define CONFIG_H

#include <QDialog>
#include <QDebug>
#include <QCheckBox>
#include <QSqlRecord>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>
#include <QSqlTableModel>
#include <QSqlRecord>

namespace Ui {
class Config;
}

class Config : public QDialog
{
    Q_OBJECT

public:
    explicit Config(QWidget *parent = nullptr);
    ~Config();

private slots:
    void on_btn_back_clicked();

    void on_btn_add_clicked();

    void on_btn_delete_clicked();

    void on_btn_save_clicked();

    void on_btn_clear_clicked();

private:
    Ui::Config *ui;

    void loadDataFromDatabase();
    void inittableWidget();
    QSqlDatabase DB;

};

#endif // CONFIG_H
