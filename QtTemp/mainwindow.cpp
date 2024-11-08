#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<QVBoxLayout>
#include<QFileDialog>
#include<QThread>
#include<QValueAxis>
#include<QLineSeries>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    InitUI();
}

MainWindow::~MainWindow()
{
    delete ui;
    if (m_thread && m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait();
    }
}

void MainWindow::InitUI()
{
    //设置view
    m_view=new QChartView(this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);


    //设置图表
    m_charts=new QChart();
    m_view->setChart(m_charts);
    m_charts->setTitle("温度曲线");

    //设置坐标轴
    QValueAxis *axisX=new QValueAxis(this);
    axisX->setTitleText("时间");
    axisX->setRange(0,100);
    axisX->setLabelsVisible(false);
    QValueAxis *axisY=new QValueAxis(this);
    axisY->setRange(-50,100);
    axisY->setTitleText("温度(摄氏度)");


    //初始化序列曲线
    QLineSeries *series0=new QLineSeries(this);
    QLineSeries *series1=new QLineSeries(this);
    m_charts->addAxis(axisX,Qt::AlignBottom);
    m_charts->addAxis(axisY,Qt::AlignLeft);
    m_charts->addSeries(series0);
    m_charts->addSeries(series1);

    series0->attachAxis(axisX);
    series0->attachAxis(axisY);
    series0->setName("最低温");

    series1->attachAxis(axisX);
    series1->attachAxis(axisY);
    series1->setName("最高温");

    series0->setPointLabelsFormat("@yPoint");
    series0->setPointLabelsVisible(true);

    series1->setPointLabelsFormat("@yPoint");
    series1->setPointLabelsVisible(true);

    series0->setPointsVisible(true);
    series1->setPointsVisible(true);

    //连接信号、设置窗体等
    connect(&m_data, &TData::error, this, &MainWindow::do_error, Qt::QueuedConnection);
    QVBoxLayout *mainLayout=new QVBoxLayout(this);
    DateSelect=new QDateTimeEdit(this);
    DateSelect->setVisible(false);
    mainLayout->addWidget(m_view);
    this->setLayout(mainLayout);
    this->setCentralWidget(m_view);
    this->resize(1200,800);
}

//将从数据库中检索回来的数据显示到界面
void MainWindow::displayData(const std::vector<Record> &vec)
{
    QLineSeries *minseries = dynamic_cast<QLineSeries*>(m_charts->series().at(0));
    QLineSeries *maxseries = dynamic_cast<QLineSeries*>(m_charts->series().at(1));

    // 清空当前的系列数据
    minseries->clear();
    maxseries->clear();
    int len=vec.size();
    if (len == 1) {
        minseries->append(0, vec[0].mintemp);
        maxseries->append(0, vec[0].maxtemp);
    }
    else
    {
        // 设置横轴的范围
        QValueAxis *axis = dynamic_cast<QValueAxis*>(m_charts->axes(Qt::Horizontal).at(0));
        if(len>=100)
            axis->setRange(0.0, vec.size() / 100);
        else
            axis->setRange(0,len);
        // m_bar.setWindowTitle("正在加载数据");
        // m_bar.resize(500,100);
        // m_bar.show();
        // std::shared_ptr<std::vector<Record>>p=std::make_shared<std::vector<Record>>(vec);
        // auto fun=[this,p,minseries,maxseries]()
        // {
        //     double x0=0.0;
        //     int i=0;
        //     QList<QPointF>mindata,maxdata;
        //     int total=p->size();
        //     for(const Record &rec:*p)
        //     {
        //         mindata.emplaceBack(x0,rec.mintemp);
        //         maxdata.emplaceBack(x0,rec.maxtemp);
        //         x0+=0.01;
        //         i++;
        //         if((i%100)==0){
        //             QMetaObject::invokeMethod(this, [this,i,total,minseries, maxseries,mindata,maxdata](){
        //                 minseries->append(mindata);
        //                 maxseries->append(maxdata);
        //                 this->m_bar.setRange(0,total);
        //                 this->m_bar.setValue(i);
        //             }, Qt::QueuedConnection);
        //             mindata.clear();
        //             maxdata.clear();
        //             std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //         }
        //     }
        //     if(mindata.size()>0)
        //     QMetaObject::invokeMethod(this, [this, minseries, maxseries, mindata, maxdata,i]() {
        //             // 更新图表数据
        //             minseries->append(mindata);
        //             maxseries->append(maxdata);
        //             this->m_bar.setValue(i);
        //     }, Qt::QueuedConnection);
        // };
        // QtConcurrent::run(fun);

        QList<QPointF>mindata,maxdata;
        double x0=0.0;
        for(const Record &rec:vec)
        {
            mindata.emplaceBack(x0,rec.mintemp);
            maxdata.emplaceBack(x0,rec.maxtemp);
            if(len<=100)
                x0+=1;
            else
                x0+=0.01;
        }
        minseries->replace(mindata);
        maxseries->replace(maxdata);
    }
}

//进行槽函数，显示进度条
void MainWindow::do_loadProgress(size_t total, size_t loaded)
{
    m_bar.setRange(0,total);
    m_bar.setValue(loaded);
}

//显示错误
void MainWindow::do_error(QString error)
{
    QMessageBox::warning(this,"error",error);
}

//初始化数据库按钮槽
void MainWindow::on_act_initdb_triggered()
{
    QString filepath=QFileDialog::getOpenFileName(this,"选择数据库文件",QDir::currentPath(),"*.db");
    if(m_data.initDatabase(filepath))   //如果为真，将加载数据按键设置为可用
    {
        ui->act_loaddata->setEnabled(true);
        ui->act_select->setEnabled(true);
        ui->act_avg->setEnabled(true);
    }
}

//加载数据按钮槽
void MainWindow::on_act_loaddata_triggered()
{
    // 检查是否已经有线程在运行
    if (m_thread && m_thread->isRunning()) {
        QMessageBox::warning(this, "Warning", "Data loading is already in progress.");
        return;
    }

    QString filepath = QFileDialog::getOpenFileName(this, "选择数据文件", QDir::currentPath(), "*.txt");
    if (filepath.isEmpty()) {
        return;  // 如果用户没有选择文件，直接返回
    }
    m_data.setDataPath(filepath);

    m_thread = new QThread();  // 创建新线程
    connect(m_thread, &QThread::started, &m_data, &TData::InSertRecords, Qt::QueuedConnection);
    connect(m_thread, &QThread::started, &m_bar, &QProgressBar::show, Qt::QueuedConnection);
    connect(&m_data, &TData::LoadProgress, this, &MainWindow::do_loadProgress, Qt::QueuedConnection);
    connect(&m_data, &TData::workFinished, m_thread, &QThread::quit, Qt::QueuedConnection);
    connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater, Qt::QueuedConnection);  // 删除线程
    m_data.moveToThread(m_thread);  // 将TData对象移到新线程中

    m_thread->start();  // 启动线程
}

//温度查询，任选时间段
void MainWindow::on_act_select_triggered()
{
    DateSelectDialog dia;
    if(dia.exec()==QDialog::Accepted)
    {
        std::pair<QDateTime,QDateTime> time=dia.GetTime();
        qDebug()<<time.first.toString("yyyy-mm-dd hh:mm:ss")<<"  "<<time.second.toString("yyyy-mm-dd hh:mm:ss");
        qint64 start=time.first.toSecsSinceEpoch();
        qint64 end=time.second.toSecsSinceEpoch();
        std::vector<Record>ret=m_data.SelectTemp("SZ",start,end);
        if(ret.size()==0)
            return;
        else
        {
            displayData(ret);
        }
    }
}

//查询指定时间段平均月温度
void MainWindow::on_act_avg_triggered()
{
    DateSelectDialog dia;
    if(dia.exec()==QDialog::Accepted)
    {
        std::pair<QDateTime,QDateTime> time=dia.GetTime();
        qint64 start=time.first.toSecsSinceEpoch();
        qint64 end=time.second.toSecsSinceEpoch();
        std::vector<Record>ret=m_data.SelectAvgTemp("SZ",start,end);
        if(ret.size()==0)
            return;
        else
        {
            displayData(ret);
        }
    }
}

