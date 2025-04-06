#include "init.h"
#include "ui_init.h"

INIT::INIT(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::INIT)
{
    ui->setupUi(this);

    ui->stackedWidget->setCurrentIndex(0);

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
    }
    else
    {
        qDebug() << "无法打开数据库：" << DB.lastError().text();
    }

    QSqlQuery query(DB);
    query.prepare("SELECT COUNT(*) FROM robots");

    if (query.exec()) {
        if (query.next()) {
            robot_num = query.value(0).toInt();
            qDebug() << "Number of rows in 'robots' table:" << robot_num;
        }
    } else {
        qDebug() << "Query execution failed:" << query.lastError().text();
    }

    ui->tableWidget_2->verticalHeader()->setVisible(false);
    ui->tableWidget_2->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_2->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget_2->horizontalHeader()->setFixedHeight(50);
    ui->tableWidget_2->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel); //设置为像素移动
    QScrollBar * a = ui->tableWidget_2->verticalScrollBar(); //获取到tablewidget的滚动条
    a->setSingleStep(5); //设置单步，值越小，下滑越慢
    connect(ui->tableWidget_2, &QTableWidget::cellClicked, this, &INIT::onCellClicked);
    for (int i = 0;i < 50; i ++) {
        ui->tableWidget_2->setRowHeight(i, 40);
        ui->tableWidget_2->item(i,0)->setFlags(ui->tableWidget_2->item(i, 0)->flags() & ~Qt::ItemIsEditable);
    }
    ui->tableWidget_3->verticalHeader()->setVisible(false);
    ui->tableWidget_3->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_3->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel); //设置为像素移动
    a=ui->tableWidget_3->horizontalScrollBar();
    a->setSingleStep(5);
    ui->tableWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel); //设置为像素移动
    a=ui->tableWidget->horizontalScrollBar();
    a->setSingleStep(5);
}

INIT::~INIT()
{
    DB.close();
    // 清空所有TCP客户端
    {
        for (QModbusTcpClient *client : modbusClient_list) {
            if (client) {
                client->disconnectDevice();
                delete client;
            }
        }
        modbusClient_list.clear();

        for (QTcpSocket *client : client_data_vis_list){
            if(client){
                client->disconnectFromHost();
                delete client;
            }
        }
        client_data_vis_list.clear();


        for (QTcpSocket *client : client_con_vis_list){
            if(client){
                client->disconnectFromHost();
                delete client;
            }
        }
        client_con_vis_list.clear();

        for (QTcpSocket *client : client_send_doc_list){
            if(client){
                client->disconnectFromHost();
                delete client;
            }
        }
        client_send_doc_list.clear();

        for (QTcpSocket *client : client_chg_rec_list){
            if(client){
                client->disconnectFromHost();
                delete client;
            }
        }
        client_chg_rec_list.clear();
    }
    delete ui;
}

/* page_init */
void INIT::on_btn_config_clicked()
{
    // 创建config窗口，并设置为模态
    Config configDialog(this);
    configDialog.setModal(true);  // 使config窗口为模态
    configDialog.exec();  // 显示窗口，并阻塞INIT界面操作，直到config关闭
}

void INIT::on_btn_new_clicked()
{
    New newDialog(this);
    newDialog.robot_num=robot_num;
    newDialog.setModal(true);
    newDialog.exec();
    if (newDialog.ispage)
    {
        ui->stackedWidget->setCurrentIndex(1);
    }

    item_name = newDialog.item_name;        // 物料名称
    item_row = newDialog.item_row;          // 物料行数
    item_cols = newDialog.item_cols;        // 物料列数
    item_wide = newDialog.item_wide;        // 物料宽度
    item_hight = newDialog.item_hight;      // 物料长度
    distance_x = newDialog.distance_x;      // 物料距离x
    distance_y = newDialog.distance_y;      // 物料距离y
    thickness_x = newDialog.thickness_x;    // x间隔厚度
    thickness_y = newDialog.thickness_y;    // y间隔厚度
    order = newDialog.order;                // 作业顺序 0为行优先，1为列优先
    direction = newDialog.direction;        // 传送带方向与机器人Y是否反向   0为不相反，1为相反
    destPath = newDialog.destPath;          // 插入图片路径
    robot_list = newDialog.robot_list;
    on_set_main_le();
}

void INIT::on_btn_open_clicked()
{
    // 打开文件选择对话框，允许用户选择一个配置文件
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Configuration File"), "", tr("INI Files (*.ini);;All Files (*)"));

    // 如果没有选择文件，直接返回
    if (fileName.isEmpty()) {
        return;
    }

    // 使用 QSettings 打开选中的配置文件
    QSettings settings(fileName, QSettings::IniFormat);

    // 读取物料配置信息
    settings.beginGroup("MaterialConfig");
    item_name = settings.value("item_name", "").toString();
    item_row = settings.value("item_row", 0).toInt();
    item_cols = settings.value("item_cols", 0).toInt();
    item_wide = settings.value("item_wide", "").toString();
    item_hight = settings.value("item_hight", "").toString();
    distance_x = settings.value("distance_x", "").toString();
    distance_y = settings.value("distance_y", "").toString();
    thickness_x = settings.value("thickness_x", "").toString();
    thickness_y = settings.value("thickness_y", "").toString();
    order = settings.value("order", 0).toBool();  // 默认值为 0
    direction = settings.value("direction", 1).toBool();  // 默认值为 1
    destPath = settings.value("destPath", "").toString();  // 获取图片路径
    settings.endGroup();

    // 读取机器人列表
    robot_list.clear();
    settings.beginGroup("RobotList");
    int robot_index = 0;
    while (settings.contains(QString("robot_%1").arg(robot_index))) {
        int robot_value = settings.value(QString("robot_%1").arg(robot_index), 0).toInt();
        robot_list.push_back(robot_value);  // 将机器人索引放入列表
        ++robot_index;
    }
    settings.endGroup();
    ui->stackedWidget->setCurrentIndex(1);
    on_set_main_le();

    // 打印调试信息
    qDebug() << "Configuration loaded from " << fileName;
    qDebug() << "Item Name: " << item_name;
    qDebug() << "Rows: " << item_row << " Columns: " << item_cols;
    qDebug() << "Width: " << item_wide << " Height: " << item_hight;
    qDebug() << "Distance X: " << distance_x << " Distance Y: " << distance_y;
    qDebug() << "Thickness X: " << thickness_x << " Thickness Y: " << thickness_y;
    qDebug() << "Order: " << order << " Direction: " << direction;
    qDebug() << "Destination Path: " << destPath;
    qDebug() << "Robot List: " << robot_list;
}

void INIT::on_btn_save_parameters_clicked()
{
    item_name = ui->le_item_name->text();
    item_row = ui->le_item_row->text().toUInt();
    item_cols = ui->le_item_cols->text().toUInt();
    item_wide = ui->le_item_wide->text();
    item_hight = ui->le_item_hight->text();
    distance_x = ui->le_distance_x->text();
    distance_y = ui->le_distance_y->text();
    thickness_x = ui->le_thickness_x->text();
    thickness_y = ui->le_thickness_y->text();
    order = (ui->cb_order->currentIndex() != 0);
    direction = ui->check_direction->isChecked();
    // destPath
    robot_list.clear();
    // 获取 `scrollAreaWidgetContents` 中的布局
    QGridLayout* layout = dynamic_cast<QGridLayout*>(ui->scrollAreaWidgetContents->layout());
    // 遍历 `QGridLayout` 中的所有控件
    for (int row = 0; row < item_row; ++row)
    {
        for (int col = 0; col < item_cols; ++col)
        {
            QLayoutItem* item = layout->itemAtPosition(row, col);
            QWidget* widget = item->widget();
            QComboBox* comboBox = qobject_cast<QComboBox*>(widget);

            robot_list.push_back(comboBox->currentIndex());     // 将当前索引放入列表
        }
    }

    // 保存到配置文件
    QString fileName = item_name + ".ini";  // 文件名以 item_name 命名
    QSettings settings(fileName, QSettings::IniFormat);

    // 保存物料信息
    settings.beginGroup("MaterialConfig");
    settings.setValue("item_name", item_name);
    settings.setValue("item_row", item_row);
    settings.setValue("item_cols", item_cols);
    settings.setValue("item_wide", item_wide);
    settings.setValue("item_hight", item_hight);
    settings.setValue("distance_x", distance_x);
    settings.setValue("distance_y", distance_y);
    settings.setValue("thickness_x", thickness_x);
    settings.setValue("thickness_y", thickness_y);
    settings.setValue("order", order);
    settings.setValue("direction", direction);
    settings.setValue("destPath", destPath);

    settings.endGroup();

    // 保存机器人列表
    settings.beginGroup("RobotList");
    for (size_t i = 0; i < robot_list.size(); ++i) {
        settings.setValue(QString("robot_%1").arg(i), robot_list[i]);
    }
    settings.endGroup();

    qDebug() << "Configuration saved to " << fileName;
}


void INIT::on_btn_back_init_clicked()
{
    ui->stackedWidget->setCurrentIndex(0);
}

void INIT::on_btn_open_map_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
}

void INIT::on_btn_back_main_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

/* page_main */

void INIT::on_set_main_le()
{
    ui->le_item_name->setText(item_name);
    ui->le_item_row->setText(QString::number(item_row));
    ui->le_item_cols->setText(QString::number(item_cols));
    ui->le_item_wide->setText(item_wide);
    ui->le_item_hight->setText(item_hight);
    ui->le_distance_x->setText(distance_x);
    ui->le_distance_y->setText(distance_y);
    ui->le_thickness_x->setText(thickness_x);
    ui->le_thickness_y->setText(thickness_y);
    ui->cb_order->setCurrentIndex(order);
    ui->check_direction->setChecked(direction);
    on_check_direction_stateChanged(direction);
    if(destPath!="")
    {
        // 将图片加载到 label_9 控件上
        QPixmap pixmap(destPath);
        if (!pixmap.isNull()) {
            // 调整图片大小以适应 label_9 控件
            ui->label_9->setPixmap(pixmap.scaled(ui->label_9->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
        }
    }else{
        ui->label_9->setText("未插入图片");
    }
    flag = true;
    on_btn_reset_order_clicked();
    on_btn_generate_parameters_clicked();
}

void INIT::on_check_direction_stateChanged(int arg1)
{
    if(arg1)
    {
        ui->label_8->setPixmap(QPixmap(":/new/prefix3/2.png"));
    }
    else
    {
        ui->label_8->setPixmap(QPixmap(":/new/prefix3/1.png"));
    }
}

void INIT::on_btn_Insert_picture_clicked()
{
    // 弹出文件选择对话框，只能选择图片文件
    QString filePath = QFileDialog::getOpenFileName(this, tr("选择图片"), "", tr("Images (*.png *.jpg *.jpeg *.bmp)"));

    if (!filePath.isEmpty()) {
        // 获取当前工作路径
        QString currentPath = QDir::currentPath()+"/Picture";

        // 检查并创建 Picture 文件夹（如果不存在）
        QDir dir(currentPath);
        if (!dir.exists()) {
            dir.mkpath(currentPath);  // 创建 Picture 文件夹
        }

        QFileInfo fileInfo(filePath);
        QString fileName = fileInfo.fileName();
        destPath = currentPath + "/" + fileName;

        // 如果文件不在当前路径下，将其复制到当前路径
        if (filePath != destPath) {
            QFile::copy(filePath, destPath);
        }

        // 将图片加载到 label_9 控件上
        QPixmap pixmap(destPath);
        if (!pixmap.isNull()) {
            // 调整图片大小以适应 label_9 控件
            ui->label_9->setPixmap(pixmap.scaled(ui->label_9->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
        }
    }
}

void INIT::on_btn_reset_order_clicked()
{
    item_row = ui->le_item_row->text().toInt();
    item_cols = ui->le_item_cols->text().toInt();
    order=(ui->cb_order->currentIndex() != 0);

    int itemCount = item_row * item_cols;
    int n = robot_num;

    // 清空 scrollArea 的布局
    QGridLayout* layout = dynamic_cast<QGridLayout*>(ui->scrollAreaWidgetContents->layout());
    if (!layout) {
        layout = new QGridLayout(ui->scrollAreaWidgetContents);
        ui->scrollAreaWidgetContents->setLayout(layout);
    } else {
        // 清空现有布局内容
        QLayoutItem* item;
        while ((item = layout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
    }

    // 根据行优先或列优先排序生成 ComboBox
    for (int i = 0; i < itemCount; ++i) {
        int row = i / item_cols;
        int col = i % item_cols;

        QComboBox* comboBox = new QComboBox(this);

        // 填充每个 ComboBox 的内容
        for (int j = 1; j <= n; ++j) {
            QString itemText = QString("物料%1机器 %2").arg(i + 1).arg(j);
            comboBox->addItem(itemText);
        }

        // 设置默认选项
        int defaultMachineIndex = order
                ? col*item_row+row >= n ? 0 : col*item_row+row
                                        : row*item_cols+col >= n ? 0 : row*item_cols+col ;

        comboBox->setCurrentIndex(defaultMachineIndex);

        // 将 ComboBox 添加到 grid layout
        layout->addWidget(comboBox, row, col);
    }
    // 在布局的右侧和底部添加弹簧（spacers）
    QSpacerItem* hSpacer = new QSpacerItem(500,0,QSizePolicy::Expanding,QSizePolicy::Minimum);
    QSpacerItem* vSpacer = new QSpacerItem(0,500,QSizePolicy::Minimum, QSizePolicy::Expanding);

    layout->addItem(hSpacer, 0, item_cols);      // 添加水平弹簧到右侧
    layout->addItem(vSpacer, item_row, 0);       // 添加垂直弹簧到底部
    if(flag)
    {
        // 遍历 `QGridLayout` 中的所有控件
        int idx = 0;
        for (int row = 0; row < item_row; ++row)
        {
            for (int col = 0; col < item_cols; ++col)
            {
                QLayoutItem* item = layout->itemAtPosition(row, col);
                QWidget* widget = item->widget();
                QComboBox* comboBox = qobject_cast<QComboBox*>(widget);
                comboBox->setCurrentIndex(robot_list[idx]);
                idx++;
            }
        }
        flag = !flag;
    }
}

void INIT::on_btn_generate_parameters_clicked()
{
    // 清空所有TCP客户端
    {
        for (QModbusTcpClient *client : modbusClient_list) {
            if (client) {
                client->disconnectDevice();
                delete client;
            }
        }
        modbusClient_list.clear();

        for (QTcpSocket *client : client_data_vis_list){
            if(client){
                client->disconnectFromHost();
                delete client;
            }
        }
        client_data_vis_list.clear();

        for (QTcpSocket *client : client_con_vis_list){
            if(client){
                client->disconnectFromHost();
                delete client;
            }
        }
        client_con_vis_list.clear();

        for (QTcpSocket *client : client_send_doc_list){
            if(client){
                client->disconnectFromHost();
                delete client;
            }
        }
        client_send_doc_list.clear();

        for (QTcpSocket *client : client_chg_rec_list){
            if(client){
                client->disconnectFromHost();
                delete client;
            }
        }
        client_chg_rec_list.clear();
    }

    // 设置表格
    {
        // 设置表格列数
        ui->tableWidget->setColumnCount(15);
        ui->tableWidget->verticalHeader()->setVisible(false);

        // 设置表头
        QStringList headers = { "物料\n编号", "机器人\n编号",
                                "X偏移\n理论值", "X偏移\n偏调",
                                "Y偏移\n理论值", "Y偏移\n偏调",
                                "Z偏移\n理论值", "Z偏移\n偏调",
                                "RZ偏移\n理论值", "RZ偏移\n偏调",
                                "连接状态", "机器人\nIP",
                                "箱子X", "箱子Y", "箱子Z",};
        ui->tableWidget->setHorizontalHeaderLabels(headers);

        // 设置列宽
        for (int i = 0; i <= 7; ++i) {
            ui->tableWidget->setColumnWidth(i, 50);
        }
        ui->tableWidget->setColumnWidth(8, 55);
        ui->tableWidget->setColumnWidth(9, 55);
        ui->tableWidget->setColumnWidth(10, 60);
        ui->tableWidget->setColumnWidth(11, 75);
        for (int i = 12; i <= 14; ++i) {
            ui->tableWidget->setColumnWidth(i, 50);
        }
    }

    // 清空表格的行
    ui->tableWidget->setRowCount(0);

    // 获取 `scrollAreaWidgetContents` 中的布局
    QGridLayout* layout = dynamic_cast<QGridLayout*>(ui->scrollAreaWidgetContents->layout());

    item_wide = ui->le_item_wide->text();
    item_hight = ui->le_item_hight->text();
    distance_x = ui->le_distance_x->text();
    distance_y = ui->le_distance_y->text();
    thickness_x = ui->le_thickness_x->text();
    thickness_y = ui->le_thickness_y->text();

    // 初始化表格行计数器
    int tableRow = 0;

    // 遍历 `QGridLayout` 中的所有控件
    for (int row = 0; row < item_row; ++row)
    {
        for (int col = 0; col < item_cols; ++col)
        {
            QLayoutItem* item = layout->itemAtPosition(row, col);
            QWidget* widget = item->widget();
            QComboBox* comboBox = qobject_cast<QComboBox*>(widget);

            // 获取 `QComboBox` 的当前选项文本
            QString currentText = comboBox->currentText();

            // 使用正则表达式提取 `x` 和 `n`
            QRegularExpression re("物料(\\d+)机器 (\\d+)");
            QRegularExpressionMatch match = re.match(currentText);

            QString material = match.captured(1); // 提取 `x`，即物料编号
            QString robotID = match.captured(2);  // 提取 `n`，即机器编号

            // 查询数据库中的数据
            QSqlQuery query(DB);
            query.prepare("SELECT X, Y, Z, IP FROM robots WHERE id = :robotID");
            query.bindValue(":robotID", robotID);
            query.exec(),query.next();
            // 获取查询结果
            QString x = query.value(0).toString();
            QString y = query.value(1).toString();
            QString z = query.value(2).toString();
            QString ip = query.value(3).toString();

            ui->tableWidget->insertRow(tableRow);  // 插入新行

            // 计算偏移值
            float x_offset = distance_x.toFloat()+(item_row-row-1)*(thickness_x.toFloat()+item_wide.toFloat())+item_wide.toFloat() / 2;
            float y_offset = distance_y.toFloat()+(item_cols-col-1)*(thickness_y.toFloat()+item_hight.toFloat())+item_hight.toFloat() / 2;

            // 将内容填入表格中的对应单元格
            {
                ui->tableWidget->setItem(tableRow, 0, new QTableWidgetItem(material));
                ui->tableWidget->setItem(tableRow, 1, new QTableWidgetItem(robotID));
                ui->tableWidget->setItem(tableRow, 12, new QTableWidgetItem(x));
                ui->tableWidget->setItem(tableRow, 13, new QTableWidgetItem(y));
                ui->tableWidget->setItem(tableRow, 14, new QTableWidgetItem(z));
                ui->tableWidget->setItem(tableRow, 11, new QTableWidgetItem(ip));
                ui->tableWidget->setItem(tableRow, 10, new QTableWidgetItem("未连接"));
                ui->tableWidget->setItem(tableRow, 3, new QTableWidgetItem("0"));
                ui->tableWidget->setItem(tableRow, 5, new QTableWidgetItem("0"));
                ui->tableWidget->setItem(tableRow, 7, new QTableWidgetItem("0"));
                ui->tableWidget->setItem(tableRow, 9, new QTableWidgetItem("0"));

                ui->tableWidget->setItem(tableRow, 2, new QTableWidgetItem(QString::number(x_offset, 'g', -1)));
                ui->tableWidget->setItem(tableRow, 4, new QTableWidgetItem(QString::number(y_offset, 'g', -1)));
                ui->tableWidget->setItem(tableRow, 6, new QTableWidgetItem("0"));
                ui->tableWidget->setItem(tableRow, 8, new QTableWidgetItem("0"));
            }
            // 设置某些列不可编辑
            {
                ui->tableWidget->item(tableRow,0)->setFlags(ui->tableWidget->item(tableRow, 0)->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->item(tableRow,1)->setFlags(ui->tableWidget->item(tableRow, 1)->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->item(tableRow,2)->setFlags(ui->tableWidget->item(tableRow, 2)->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->item(tableRow,4)->setFlags(ui->tableWidget->item(tableRow, 4)->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->item(tableRow,6)->setFlags(ui->tableWidget->item(tableRow, 6)->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->item(tableRow,8)->setFlags(ui->tableWidget->item(tableRow, 8)->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->item(tableRow,10)->setFlags(ui->tableWidget->item(tableRow, 10)->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->item(tableRow,11)->setFlags(ui->tableWidget->item(tableRow, 11)->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->item(tableRow,12)->setFlags(ui->tableWidget->item(tableRow, 12)->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->item(tableRow,13)->setFlags(ui->tableWidget->item(tableRow, 13)->flags() & ~Qt::ItemIsEditable);
                ui->tableWidget->item(tableRow,14)->setFlags(ui->tableWidget->item(tableRow, 14)->flags() & ~Qt::ItemIsEditable);
            }
            // 设置某些列背景颜色
            {
                ui->tableWidget->item(tableRow,0)->setBackground(QColor(119, 136, 153));
                ui->tableWidget->item(tableRow,1)->setBackground(QColor(119, 136, 153));
                ui->tableWidget->item(tableRow,2)->setBackground(QColor(119, 136, 153));
                ui->tableWidget->item(tableRow,4)->setBackground(QColor(119, 136, 153));
                ui->tableWidget->item(tableRow,6)->setBackground(QColor(119, 136, 153));
                ui->tableWidget->item(tableRow,8)->setBackground(QColor(119, 136, 153));
                ui->tableWidget->item(tableRow,10)->setBackground(QColor(119, 136, 153));
                ui->tableWidget->item(tableRow,11)->setBackground(QColor(119, 136, 153));
                ui->tableWidget->item(tableRow,12)->setBackground(QColor(119, 136, 153));
                ui->tableWidget->item(tableRow,13)->setBackground(QColor(119, 136, 153));
                ui->tableWidget->item(tableRow,14)->setBackground(QColor(119, 136, 153));
            }
            // 创建所需要的通讯连接
            {
                QModbusTcpClient *client = new QModbusTcpClient();
                client->setConnectionParameter(QModbusDevice::NetworkAddressParameter, ip);
                client->setConnectionParameter(QModbusDevice::NetworkPortParameter, 502);
                modbusClient_list.push_back(client);

                QTcpSocket *client_data_vis = new QTcpSocket();
                client_data_vis_list.push_back(client_data_vis);

                QTcpSocket *client_con_vis = new QTcpSocket();
                client_con_vis_list.push_back(client_con_vis);

                QTcpSocket *client_send_doc = new QTcpSocket();
                client_send_doc_list.push_back(client_send_doc);

                QTcpSocket *client_chg_rec= new QTcpSocket();
                client_chg_rec_list.push_back(client_chg_rec);
            }

            // 移动到表格的下一行
            tableRow++;
        }
    }
}


void INIT::on_btn_con_clicked()
{
    for (int idx = 0; idx < item_row * item_cols; idx++)
    {
        // 尝试连接
        if (!modbusClient_list[idx]->connectDevice()) {
            qDebug() << "Connection failed!";
        }
        else
        {
            ui->tableWidget->setItem(idx,10,new QTableWidgetItem("已连接"));
        }
    }
    if(ui->checkBox_2->isChecked())
    {
        for(int idx = 0; idx < item_row * item_cols; idx++)
        {
            QTcpSocket* client = client_data_vis_list[idx];
            QString ip = ui->tableWidget->item(idx,11)->text();
            client->connectToHost(ip, 512);
            if (!client->waitForConnected(5000)) { // 等待连接，最多3秒
                QMessageBox::critical(this, "错误","连接失败："+ip);
                ui->tableWidget->setItem(idx,10,new QTableWidgetItem("未连接"));
            }else{
                ui->tableWidget->setItem(idx,10,new QTableWidgetItem("已连接"));
            }
        }
    }
}

void INIT::on_btn_discon_clicked()
{
    for (int idx = 0; idx < item_row * item_cols; idx++)
    {
        if (modbusClient_list[idx]) modbusClient_list[idx]->disconnectDevice(),ui->tableWidget->setItem(idx,10,new QTableWidgetItem("未连接"));
    }
}

void INIT::on_ben_send_clicked()
{
    for (int idx = 0; idx < item_row * item_cols; idx++)
    {
        if (!modbusClient_list[idx])
            continue;

        // 创建要写入的寄存器请求，写入到寄存器地址 0，写入 1 个寄存器
        QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, 2, 1);

        // 设置寄存器的值，假设我们写入 16256
        writeUnit.setValue(0, 16256);

        // 发送写入请求
        QModbusReply *reply = modbusClient_list[idx]->sendWriteRequest(writeUnit, 1); // 1 是 Modbus 设备的单元 ID
        if (reply) {
            if (!reply->isFinished()) {
                connect(reply, &QModbusReply::finished, this, [reply, idx]() {
                    if (reply->error() == QModbusDevice::NoError) {
                        qDebug() << "Write request successful for client at index:" << idx;
                    } else {
                        qDebug() << "Write error for client at index:" << idx << ":" << reply->errorString();
                    }
                    reply->deleteLater(); // 正确释放回复对象，避免内存泄漏
                });
            } else {
                // 如果回复立即返回（非异步），则直接处理并释放
                if (reply->error() == QModbusDevice::NoError) {
                    qDebug() << "Write request immediately successful for client at index:" << idx;
                } else {
                    qDebug() << "Immediate write error for client at index:" << idx << ":" << reply->errorString();
                }
                reply->deleteLater();
            }
        } else {
            qDebug() << "Failed to send write request for client at index:" << idx << ":" << modbusClient_list[idx]->errorString();
        }
        QByteArray data;
        data.append("Head,");
        data.append(ui->tableWidget->item(idx, 0)->text().toUtf8());
        data.append(",");
        data.append(QString::number(item_row * item_cols).toUtf8());
        data.append(",");
        data.append(ui->tableWidget->item(idx, 1)->text().toUtf8());
        data.append(",End");
        client_data_vis_list[idx]->write(data);
    }
}

/* page_map */
void INIT::onCellClicked(int row)
{
    ch_row = row;
}
void INIT::on_btn_import_clicked()
{
    ui->tableWidget_2->setItem(ch_row, 1, new QTableWidgetItem(item_name));
}

void INIT::on_btn_clear_selected_clicked()
{
    // 获取所有选中的项目
    QList<QTableWidgetItem*> selectedItems = ui->tableWidget_2->selectedItems();
    if (!selectedItems.isEmpty()) {
        int row = selectedItems.first()->row(); // 获取第一项的行号
        ui->tableWidget_2->setItem(row, 1, new QTableWidgetItem(""));
        ui->tableWidget_2->setItem(row, 2, new QTableWidgetItem(""));
    }
}

void INIT::on_btn_save_clicked()
{
    // 检查物料名称列是否有重复项
    QSet<QString> itemNames;
    bool hasDuplicates = false;

    // 遍历表格中的所有行
    for (int row = 0; row < ui->tableWidget_2->rowCount(); ++row) {
        QTableWidgetItem *itemNameItem = ui->tableWidget_2->item(row, 1);
        if (itemNameItem && !itemNameItem->text().isEmpty()) {
            QString itemName = itemNameItem->text();

            // 检查是否有重复的物料名称
            if (itemNames.contains(itemName)) {
                hasDuplicates = true;
                qDebug() << "重复的物料名称:" << itemName;
                return; // 直接返回，避免保存
            }

            itemNames.insert(itemName);

            // 如果找到与 item_name 相同的物料名称，更新 lab_item_name 和 lab_vision_name
            if (itemName == item_name) {
                QTableWidgetItem *visionNameItem = ui->tableWidget_2->item(row, 2);
                QString visionName = visionNameItem ? visionNameItem->text()+".sol" : "";

                ui->lab_item_name->setText(itemName);
                ui->lab_vision_name->setText(visionName);

                qDebug() << "更新 lab_item_name 和 lab_vision_name 为:" << itemName << visionName;
            }
        }
    }

    if (hasDuplicates) {
        qDebug() << "检测到重复的物料名称，保存失败！";
        return;
    }

    // 保存表格内容到内存
    tableData.clear();
    for (int row = 0; row < ui->tableWidget_2->rowCount(); ++row) {
        QVector<QString> rowData;
        for (int col = 0; col < ui->tableWidget_2->columnCount(); ++col) {
            QTableWidgetItem *item = ui->tableWidget_2->item(row, col);
            rowData.append(item ? item->text() : "");
        }
        tableData.append(rowData);
    }

    qDebug() << "表格内容已保存";
}

void INIT::on_btn_flushed_clicked()
{
    // 检查是否有保存的数据
    if (tableData.isEmpty()) {
        qDebug() << "没有可用的保存数据，无法刷新";
        return;
    }

    // 使用保存的数据覆盖表格内容
    for (int row = 0; row < tableData.size(); ++row) {
        for (int col = 0; col < tableData[row].size(); ++col) {
            ui->tableWidget_2->setItem(row, col, new QTableWidgetItem(tableData[row][col]));
        }
    }

    qDebug() << "表格内容已刷新";
}

void INIT::on_btn_con_vis_clicked()
{
    if(ui->lab_connect->text() == "全部连接")
    {
        for (QTcpSocket *client : client_con_vis_list){
            if(client){
                client->disconnectFromHost();
            }
        }
        ui->lab_connect->setText("未连接");
        QPalette palette = ui->lab_connect->palette();
        palette.setColor(QPalette::WindowText, QColor(0, 0, 0));  // 设置文本颜色
        ui->lab_connect->setPalette(palette);
        ui->btn_con_vis->setText("连接视觉终端");
    }else
    {
        // 遍历 client_con_vis_list 中的所有 QTcpSocket
        for (int i = 0; i < item_row * item_cols; ++i)
        {
            QTcpSocket* client = client_con_vis_list[i];
            QString ip = ui->tableWidget->item(i,11)->text();
            client->connectToHost(ip, 522);
            if (!client->waitForConnected(5000)) { // 等待连接，最多3秒
                QString currentTime = QDateTime::currentDateTime().toString("HH:mm:ss yyyy-MM-dd");
                // 插入数据到 tableWidget_3 的第一行
                ui->tableWidget_3->insertRow(0);
                ui->tableWidget_3->setItem(0, 0, new QTableWidgetItem(currentTime)); // 第一列：时间
                QString msgText = QString("第 %1 个物料的机器人连接失败，IP：%2; port: 522").arg(i + 1).arg(ip);
                ui->tableWidget_3->setItem(0, 1, new QTableWidgetItem(msgText)); // 第二列：消息

                ui->lab_connect->setText("连接失败");
                QPalette palette = ui->lab_connect->palette();
                palette.setColor(QPalette::WindowText, QColor(255, 0, 0));  // 设置文本颜色
                ui->lab_connect->setPalette(palette);
                return; // 终止函数，避免继续执行
            }
            client->write("INIT");                                         // 要发送的 16 进制数据：04 49 4E 49 54

            // 使用 lambda 表达式处理 readyRead 信号
            connect(client, &QTcpSocket::readyRead, this, [client, i, ip, this]()
            {
                QByteArray data = client->readAll();
                QString message = QString::fromUtf8(data);
                // 获取当前时间字符串
                QString currentTime = QDateTime::currentDateTime().toString("HH:mm:ss yyyy-MM-dd");
                if("INITTRUE" == message)
                {
                    // 插入数据到 tableWidget_3 的第一行
                    ui->tableWidget_3->insertRow(0);
                    ui->tableWidget_3->setItem(0, 0, new QTableWidgetItem(currentTime)); // 第一列：时间
                    QString msgText = QString("第 %1 个物料的机器人连接成功，IP：%2; port: 522").arg(i + 1).arg(ip);
                    ui->tableWidget_3->setItem(0, 1, new QTableWidgetItem(msgText)); // 第二列：消息
                }

                // 调试信息输出
                qDebug() << "Inserted new row: Time =" << currentTime << ", Message =" << message;
            });
        }
        ui->lab_connect->setText("全部连接");
        QPalette palette = ui->lab_connect->palette();
        palette.setColor(QPalette::WindowText, QColor(0, 128, 0));  // 绿色文本
        ui->lab_connect->setPalette(palette);
        ui->btn_con_vis->setText("断开视觉连接");
    }
}

void INIT::on_btn_send_doc_clicked()
{
    // 05
    // 54 52 41 4E 53  + 文件名大小 +文件名 +文件数据大小的字节数 53 49 5A 45+ "," + 文件大小
    // 文件内容
    // 打开文件选择对话框
    for (int i = 0; i < item_row * item_cols; ++i)
    {
        QTcpSocket* client = client_con_vis_list[i];
        // 检查客户端连接状态
        if (client->state() != QAbstractSocket::ConnectedState) {
            QString ip = ui->tableWidget->item(i, 11)->text();
            QString errorMessage = QString("请检查未连接通讯的机器人!");

            // 弹窗报错信息
            QMessageBox::critical(this, "错误", errorMessage);
            return; // 终止函数，避免继续执行
        }
    }

    QString filePath = QFileDialog::getOpenFileName(nullptr, tr("选择文件"), "", tr("VM配方 (*.sol *.xml)"));

    if (filePath.isEmpty()) {
        qDebug() << "未选择文件";
        return;
    }

    // 获取文件信息
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName(); // 文件名
    qint64 fileSize = fileInfo.size();      // 文件大小
    QString fileSizeStr = QString::number(fileSize); // 转换为字符串
    // 读取文件内容
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开文件";
        return;
    }
    QByteArray dataToSend1;
    dataToSend1.append(0x05);                               // 添加起始标志 0x05
    QByteArray dataToSend2;
    dataToSend2.append("TRANS");                            // 添加 "TRANS" 字符串
    dataToSend2.append(static_cast<char>(fileName.size())); // 添加文件名大小
    dataToSend2.append(fileName.toUtf8());                  // 添加文件名
    dataToSend2.append(static_cast<char>(fileSizeStr.size())+5);//
    dataToSend2.append("SIZE");
    dataToSend2.append(",");                                // 添加分隔符
    dataToSend2.append(QString::number(fileSize).toUtf8()); // 添加文件大小

    const int chunkSize = 8192; // 分块大小：8 KB，可以根据需要调整
    QByteArray buffer;

    while (!file.atEnd()) {
        buffer = file.read(chunkSize); // 读取一块数据
        if (buffer.isEmpty()) {
            qDebug() << "读取文件时出现错误";
            break;
        }
        // 将读取到的数据块追加到 dataToSend2 中
        dataToSend2.append(buffer);
    }

    for (int i = 0; i < item_row * item_cols; ++i)
    {
        QTcpSocket* client = client_send_doc_list[i];
        QString ip = ui->tableWidget->item(i,11)->text();
        client->connectToHost(ip, 522);
        client->write(dataToSend1);
        client->write(dataToSend2);
        connect(client, &QTcpSocket::readyRead, this, [client, i, filePath, this]()
        {
            QByteArray data = client->readAll();
            QString message = QString::fromUtf8(data);
            // 获取当前时间字符串
            QString currentTime = QDateTime::currentDateTime().toString("HH:mm:ss yyyy-MM-dd");
            if("\u0006TFTRUE" == message)
            {
                // 插入数据到 tableWidget_3 的第一行
                ui->tableWidget_3->insertRow(0);
                ui->tableWidget_3->setItem(0, 0, new QTableWidgetItem(currentTime)); // 第一列：时间
                QString msgText = QString("机器人 %1 下发文件完成， %2 ").arg(i + 1).arg(filePath);
                ui->tableWidget_3->setItem(0, 1, new QTableWidgetItem(msgText)); // 第二列：消息
            }else if("\u0007CHFALSE" == message){
                // 插入数据到 tableWidget_3 的第一行
                ui->tableWidget_3->insertRow(0);
                ui->tableWidget_3->setItem(0, 0, new QTableWidgetItem(currentTime)); // 第一列：时间
                QString msgText = QString("第 %1 个物料机器人方案切换失败 %2 ").arg(i + 1).arg(ui->lab_vision_name->text());
                ui->tableWidget_3->setItem(0, 1, new QTableWidgetItem(msgText)); // 第二列：消息
            }

            // 调试信息输出
            qDebug() << "Inserted new row: Time =" << currentTime << ", Message =" << message;
        });
    }
}

void INIT::on_btn_chg_rec_clicked()
{
    for (int i = 0; i < item_row * item_cols; ++i)
    {
        QTcpSocket* client = client_con_vis_list[i];
        // 检查客户端连接状态
        if (client->state() != QAbstractSocket::ConnectedState) {
            QString ip = ui->tableWidget->item(i, 11)->text();
            QString errorMessage = QString("请检查未连接通讯的机器人!");

            // 弹窗报错信息
            QMessageBox::critical(this, "错误", errorMessage);
            return; // 终止函数，避免继续执行
        }
    }

    QByteArray dataToSend;                                          // 准备发送的数据
    dataToSend.append("CHANGE");                                    // 添加 "CHANGE" 字符串
    QByteArray visionName = ui->lab_vision_name->text().toUtf8();   // 获取方案名并转换为 UTF-8 编码
    int nameSize = visionName.size();                               // 获取方案名的字节大小并添加为 1 字节（假设不会超过 255 字节）
    dataToSend.append(static_cast<char>(nameSize));                 // 添加方案名大小
    dataToSend.append(visionName);                                  // 添加方案名

    // 遍历 client_chg_rec_list 中的所有 QTcpSocket
    for (int i = 0; i < item_row * item_cols; ++i)
    {
        QTcpSocket* client = client_chg_rec_list[i];
        QString ip = ui->tableWidget->item(i,11)->text();
        client->connectToHost(ip, 522);
        if (!client->waitForConnected(3000)) { // 等待连接，最多3秒
            QString errorMessage = QString("客户端索引 %1 (IP: %2) 连接失败，请检查网络连接。").arg(i).arg(ip);
            QMessageBox::critical(this, "连接错误", errorMessage);
            return; // 终止函数，避免继续执行
        }
        QString currentTime = QDateTime::currentDateTime().toString("HH:mm:ss yyyy-MM-dd");
        // 插入数据到 tableWidget_3 的第一行
        ui->tableWidget_3->insertRow(0);
        ui->tableWidget_3->setItem(0, 0, new QTableWidgetItem(currentTime)); // 第一列：时间
        QString msgText = QString("第 %1 个物料的机器人，开始切换").arg(i + 1);
        ui->tableWidget_3->setItem(0, 1, new QTableWidgetItem(msgText)); // 第二列：消息
        client->write("");                   // 06
        client->write(dataToSend);           // 43 48 41 4E 47 45(CHANGE)+ 方案名大小  + 方案名
        // 使用 lambda 表达式处理 readyRead 信号
        connect(client, &QTcpSocket::readyRead, this, [client, i, this]()
        {
            QByteArray data = client->readAll();
            QString message = QString::fromUtf8(data);
            // 获取当前时间字符串
            QString currentTime = QDateTime::currentDateTime().toString("HH:mm:ss yyyy-MM-dd");
            if("\u0006CHTRUE" == message)
            {
                // 插入数据到 tableWidget_3 的第一行
                ui->tableWidget_3->insertRow(0);
                ui->tableWidget_3->setItem(0, 0, new QTableWidgetItem(currentTime)); // 第一列：时间
                QString msgText = QString("第 %1 个物料的机器人，切换视觉工程成功， %2 ").arg(i + 1).arg(ui->lab_vision_name->text());
                ui->tableWidget_3->setItem(0, 1, new QTableWidgetItem(msgText)); // 第二列：消息
            }else if("\u0007CHFALSE" == message){
                // 插入数据到 tableWidget_3 的第一行
                ui->tableWidget_3->insertRow(0);
                ui->tableWidget_3->setItem(0, 0, new QTableWidgetItem(currentTime)); // 第一列：时间
                QString msgText = QString("第 %1 个物料的机器人，切换视觉工程失败 %2 ").arg(i + 1).arg(ui->lab_vision_name->text());
                ui->tableWidget_3->setItem(0, 1, new QTableWidgetItem(msgText)); // 第二列：消息
            }

            // 调试信息输出
            qDebug() << "Inserted new row: Time =" << currentTime << ", Message =" << message;
        });
    }
}
