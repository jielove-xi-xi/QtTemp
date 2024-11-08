#ifndef TDATA_H
#define TDATA_H

#include <QObject>
#include<QSql>
#include<QSqlDatabase>
#include<QMessageBox>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include<QFile>


struct Record{
    QString time;
    double mintemp;
    double maxtemp;
    Record(const QString &_time,const double &_min,const double &_max):time(_time), mintemp(_min),maxtemp(_max){}
    Record()=default;
};

//数据处理类，保存和读取数据
class TData:public QObject
{
    Q_OBJECT
public:
    TData();
    bool initDatabase(const QString &dbpath);       //传递数据库路径，初始化sql数据库
    void setDataPath(const QString &path);          //设置数据文件路径
    void InSertRecords();                           //插入数据
    std::vector<Record>SelectTemp(const QString &district,const qint64 &start,const qint64 &end);    //传递地区和开始时间和结束时间检索返回数据
    std::vector<Record>SelectAvgTemp(const QString &district, const qint64 &start, const qint64 &end);
private:
    void InsertRecordsFromTxT();                    //插入数据来自txt，多行
    qint64 CountTxtRow(QFile &file);                //大体估算行数
    int64_t parseDateTime(const QString &dateStr);  //手动解析字符串成时间戳
signals:
    void LoadProgress(size_t countNum,size_t loadedcount);          //加载数据进度
    void error(QString message);                                    //发送错误
    void workFinished();                                            //多线程操作完成提示
    void workOK();                                                  //正常完成工作
private:
    QSqlDatabase m_db;    //数据库对象
    QString datapath;
};

#endif // TDATA_H
