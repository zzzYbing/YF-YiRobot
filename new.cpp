#include "new.h"
#include "ui_new.h"

New::New(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::New)
{
    ui->setupUi(this);

    ui->stackedWidget->setCurrentIndex(0);
    ui->check_direction->setChecked(1);

    // 连接 btn_1_last 和 btn_2_last 到 back 槽函数
    connect(ui->btn_1_back, &QPushButton::clicked, this, &New::back);
    connect(ui->btn_2_back, &QPushButton::clicked, this, &New::back);
}

New::~New()
{
    delete ui;
}

void New::back()
{
    this->close();  // 关闭窗口
}

/* page_1 */

void New::on_btn_1_next_clicked()
{
    item_name = ui->le_item_name->text();
    item_row = ui->le_row->text().toInt();
    item_cols = ui->le_cols->text().toInt();
    item_wide = ui->le_item_wide->text();
    item_hight = ui->le_item_hight->text();
    distance_x = ui->le_distance_x->text();
    distance_y = ui->le_distance_y->text();
    thickness_x = ui->le_thickness_x->text();
    thickness_y = ui->le_thickness_y->text();
    order = (ui->cb_order->currentIndex() != 0);
    direction = ui->check_direction->isChecked();
    // 切换到 page_2
    ui->stackedWidget->setCurrentIndex(1);

    init_page_2();
}

void New::on_check_direction_stateChanged(int arg1)
{
    if(arg1)
    {
        ui->label_17->setPixmap(QPixmap(":/new/prefix2/direction1.png"));
    }
    else
    {
        ui->label_17->setPixmap(QPixmap(":/new/prefix2/direction2.png"));
    }
}

/* page_2 */

void New::on_btn_2_last_clicked()
{
    // 切换回 page_1
    ui->stackedWidget->setCurrentIndex(0);
}

void New::init_page_2()
{
    int itemCount = item_row * item_cols;
    int n = robot_num;
    if(itemCount >n)
    {
        QMessageBox::warning(nullptr, "警告", "物料数量超过机器人数量！");
    }
    ui->label_19->setText("已配置机器人数量："+QString::number(n));
    ui->label_20->setText("物料数量："+QString::number(itemCount));

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
}

void New::on_btn_2_finish_clicked()
{
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
    ispage = true;
    back();
}

void New::on_pushButton_clicked()
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

        // 将图片加载到 label_18 控件上
        QPixmap pixmap(destPath);
        if (!pixmap.isNull()) {
            // 调整图片大小以适应 label_18 控件
            ui->label_18->setPixmap(pixmap.scaled(ui->label_18->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
        }
    }
}
