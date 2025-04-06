#ifndef INIT_H
#define INIT_H

#include "new.h"
#include "config.h"

#include <vector>

#include <QDateTime>
#include <QScrollBar>
#include <QTcpSocket>
#include <QMainWindow>
#include <QHostAddress>
#include <QAbstractSocket>

#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QModbusReply>
#include <QModbusDataUnit>
#include <QModbusTcpClient>

#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui { class INIT; }
QT_END_NAMESPACE

class INIT : public QMainWindow
{
    Q_OBJECT

public:
    INIT(QWidget *parent = nullptr);
    ~INIT();

public:
    QString item_name;      // 物料名称
    int item_row;           // 物料行数
    int item_cols;          // 物料列数
    QString item_wide;      // 物料宽度
    QString item_hight;     // 物料长度
    QString distance_x;     // 物料距离x
    QString distance_y;     // 物料距离y
    QString thickness_x;    // x间隔厚度
    QString thickness_y;    // y间隔厚度
    bool order = 0;         // 作业顺序 0为行优先，1为列优先
    bool direction = 1;     // 传送带方向与机器人Y是否反向   0为不相反，1为相反
    int robot_num;          // 配置机器人数量
    QString destPath;       // 插入图片路径
    std::vector<int> robot_list;
    bool flag;
    int ch_row;

public slots:
    void onCellClicked(int row);
private slots:
    void on_btn_config_clicked();

    void on_btn_new_clicked();

    void on_btn_back_init_clicked();

    void on_btn_back_main_clicked();

    void on_btn_open_map_clicked();

    void on_set_main_le();

    void on_check_direction_stateChanged(int arg1);

    void on_btn_Insert_picture_clicked();

    void on_btn_reset_order_clicked();

    void on_btn_generate_parameters_clicked();

    void on_btn_con_clicked();

    void on_btn_discon_clicked();

    void on_ben_send_clicked();

    void on_btn_import_clicked();

    void on_btn_clear_selected_clicked();

    void on_btn_save_clicked();

    void on_btn_flushed_clicked();

    void on_btn_con_vis_clicked();

    void on_btn_send_doc_clicked();

    void on_btn_chg_rec_clicked();

    void on_btn_open_clicked();

    void on_btn_save_parameters_clicked();

private:
    Ui::INIT *ui;

    QSqlDatabase DB;

    std::vector<QModbusTcpClient *> modbusClient_list;

    // 用于保存表格内容的变量
    QVector<QVector<QString>> tableData;

    std::vector<QTcpSocket*> client_data_vis_list;

    std::vector<QTcpSocket*> client_con_vis_list;
    std::vector<QTcpSocket*> client_send_doc_list;
    std::vector<QTcpSocket*> client_chg_rec_list;

};
#endif // INIT_H
