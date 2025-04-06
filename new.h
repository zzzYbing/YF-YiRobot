#ifndef NEW_H
#define NEW_H

#include "qpixmap.h"

#include <vector>

#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>

namespace Ui {
class New;
}

class New : public QDialog
{
    Q_OBJECT

public:
    bool ispage = false;
    explicit New(QWidget *parent = nullptr);
    ~New();

public:
    QString item_name;      // 物料名称
    int item_row;           // 物料行数
    int item_cols;          // 物料列数
    QString item_wide;          // 物料宽度
    QString item_hight;         // 物料长度
    QString distance_x;         // 物料距离x
    QString distance_y;         // 物料距离y
    QString thickness_x;        // x间隔厚度
    QString thickness_y;        // y间隔厚度
    bool order = 0;         // 作业顺序 0为行优先，1为列优先
    bool direction = 1;     // 传送带方向与机器人Y是否反向   0为不相反，1为相反
    int robot_num;          // 配置机器人数量
    QString destPath;       // 插入图片路径

    std::vector<int> robot_list;

private slots:
    void back();

    /* page_1 */

    void on_btn_1_next_clicked();

    void on_check_direction_stateChanged(int arg1);

    /* page_2 */
    void init_page_2();

    void on_btn_2_last_clicked();

    void on_btn_2_finish_clicked();

    void on_pushButton_clicked();

private:
    Ui::New *ui;
};

#endif // NEW_H
