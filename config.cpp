#include "config.h"
#include "ui_config.h"

#include <QProcess>

Config::Config(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Config)
{
    ui->setupUi(this);

    // 设置窗口标志，去掉关闭按钮和窗口图标
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);

    // 初始化数据库
    if (QSqlDatabase::contains("robot_db_connection")) {
        DB = QSqlDatabase::database("robot_db_connection");
    } else {
        DB = QSqlDatabase::addDatabase("QSQLITE", "robot_db_connection");
        DB.setDatabaseName(QDir::currentPath() + "\\robot.db"); // 打开数据库
    }

    if (DB.open())
    {
        qDebug() << "Database opened successfully";
        loadDataFromDatabase();
    }
    else
    {
        qDebug() << "无法打开数据库：" << DB.lastError().text();
    }
    inittableWidget();
}

Config::~Config()
{
    // 在销毁窗口时断开数据库连接
    // DB.close();
    delete ui;
}

void Config::on_btn_back_clicked()
{
    this->close();  // 关闭窗口
}

// 从数据库中加载数据并填充到 QTableWidget
void Config::loadDataFromDatabase()
{
    // 设置表格列数
    ui->tableWidget->setColumnCount(6);

    // 设置表头
    QStringList headers = {"机器人ID", "箱子X坐标", "箱子Y坐标", "箱子Z坐标", "机器人IP", "与传送带方向"};
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    // 查询数据库中的数据
    QSqlQuery query("SELECT * FROM robots", DB);
    if (query.exec())
    {
        // 清空表格的行
        ui->tableWidget->setRowCount(0);

        // 遍历查询结果并逐行插入到表格中
        int row = 0;
        while (query.next()) {
            ui->tableWidget->insertRow(row);  // 插入新行

            // 获取每一列的数据，并插入到对应的表格单元格中
            for (int col = 0; col < 5; col++) {
                ui->tableWidget->setItem(row, col, new QTableWidgetItem(query.value(col).toString()));
            }
            // 设置机器人ID列颜色为黄色
            ui->tableWidget->item(row, 0)->setBackground(QColor(255, 255, 0));

            // 创建复选框，并根据数据库中的值设置是否勾选
            QCheckBox *checkBox = new QCheckBox();
            if (query.value(5).toInt() == 1) {
                checkBox->setChecked(true);
            }
            ui->tableWidget->setCellWidget(row, 5, checkBox);
            row++;  // 增加行号
        }

        qDebug() << "数据已成功加载到表格";
    }
    else
    {
        qDebug() << "数据库查询失败：" << query.lastError().text();
    }
}

void Config::inittableWidget()
{
    ui->tableWidget->verticalHeader()->setFixedWidth(50); // 设置序列号列的宽度为50像素
    ui->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color: rgb(144, 238, 144); }");
}

void Config::on_btn_add_clicked()
{
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);  // 在表格最后新增一行

    // 自动填写机器人ID
    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
    ui->tableWidget->item(row, 0)->setBackground(QColor(255, 255, 0));

    // 设置 XYZ 坐标为 "1"
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem("1"));
    ui->tableWidget->setItem(row, 2, new QTableWidgetItem("1"));
    ui->tableWidget->setItem(row, 3, new QTableWidgetItem("1"));

    // 设置机器人IP为 "127.0.0.x"，其中 x 与序列号一致
    ui->tableWidget->setItem(row, 4, new QTableWidgetItem("127.0.0." + QString::number(row + 1)));

    // 设置与传送带方向为复选框，默认勾选
    QCheckBox *checkBox = new QCheckBox();
    checkBox->setChecked(true);
    ui->tableWidget->setCellWidget(row, 5, checkBox);
}

void Config::on_btn_delete_clicked()
{
    int row = ui->tableWidget->currentRow();
    if (row >= 0) {
        ui->tableWidget->removeRow(row);  // 删除当前选中的行
    }
}

void Config::on_btn_save_clicked()
{
    QSqlQuery query(DB);

    // 开始事务
    query.exec("BEGIN TRANSACTION");

    // 删除表中所有数据
    query.exec("DELETE FROM robots");

    // 获取表格中的数据，并逐行插入到数据库中
    for (int row = 0; row < ui->tableWidget->rowCount(); row++) {
        QString id = ui->tableWidget->item(row, 0)->text();
        QString x = ui->tableWidget->item(row, 1)->text();
        QString y = ui->tableWidget->item(row, 2)->text();
        QString z = ui->tableWidget->item(row, 3)->text();
        QString ip = ui->tableWidget->item(row, 4)->text();
        // 获取复选框的状态
        QCheckBox *checkBox = qobject_cast<QCheckBox*>(ui->tableWidget->cellWidget(row, 5));
        QString direction = checkBox->isChecked() ? "1" : "0";

        qDebug() << id << x << y << z << ip << direction;
        query.prepare("INSERT INTO robots (id, X, Y, Z, IP, direction) "
                      "VALUES (:id, :x, :y, :z, :ip, :direction)");
        query.bindValue(":id", id);
        query.bindValue(":x", x);
        query.bindValue(":y", y);
        query.bindValue(":z", z);
        query.bindValue(":ip", ip);
        query.bindValue(":direction", direction);
        query.exec();
    }
    // 提交事务
    query.exec("COMMIT");
    QMessageBox::information(this, "保存成功", "将会重启程序");
    qApp->quit();   // 或者   aApp->closeAllWindows();

    QProcess::startDetached(qApp->applicationFilePath(), QStringList());
}

void Config::on_btn_clear_clicked()
{
    ui->tableWidget->setRowCount(0);  // 清空所有行
}
