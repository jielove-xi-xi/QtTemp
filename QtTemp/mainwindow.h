#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QChart>
#include<QChartView>
#include<QProgressBar>
#include<QDateTimeEdit>
#include<QtConcurrent/QtConcurrent>
#include<dateselectdialog.h>
#include"tdata.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    void InitUI();      //初始化UI
    void displayData(const std::vector<Record>&vec);        //将从数据库中检索回来的数据显示到界面
private slots:
    void do_loadProgress(size_t total,size_t loaded);       //加载数据库数据进度槽函数，显示进度条
    void do_error(QString error);                           //显示错误
    void on_act_initdb_triggered();                         //初始化数据库
    void on_act_loaddata_triggered();                       //从文件加载数据
    void on_act_select_triggered();                         //检索数据，显示到界面
    void on_act_avg_triggered();

private:
    TData m_data;
    QChartView *m_view=nullptr;
    QChart *m_charts=nullptr;
    QThread *m_thread=nullptr;  // 线程指针
private:
    Ui::MainWindow *ui;
    QProgressBar m_bar;
    QDateTimeEdit *DateSelect=nullptr;
};

#endif // MAINWINDOW_H
