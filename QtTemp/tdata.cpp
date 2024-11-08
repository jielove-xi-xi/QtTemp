#include "tdata.h"
#include<QThread>
#include<QCoreApplication>
#include<QFileInfo>

TData::TData() {}

//传递数据库文件路径初始化数据库，正确返回true
bool TData::initDatabase(const QString &dbpath)
{
    m_db=QSqlDatabase::addDatabase("QSQLITE");  //加载SQLite数据库
    m_db.setDatabaseName(dbpath);

    if(!m_db.open())    //如果数据库打开失败
    {
        QMessageBox::warning(nullptr,"ERROR","Database open failed:"+m_db.lastError().text());
        return false;
    }

    //创建表
    QString sqlcmd=R"(
        CREATE TABLE IF NOT EXISTS TEMP(
        DISTRICT TEXT NOT NULL,
        RECODEDATE INTEGER NOT NULL,
        MINTEMP INT NOT NULL,
        MAXTEMP INT NOT NULL,
        PRIMARY KEY (DISTRICT, RECODEDATE)  -- 复合主键
        );
    )";    //将地区和时间作为复合主键，加快查找

    QSqlQuery query;
    if(!query.exec(sqlcmd))
    {
        QMessageBox::warning(nullptr,"ERROR","Table create failed:"+query.lastError().text());
        return false;
    }
    return true;
}

void TData::InsertRecordsFromTxT()
{
    QFile file(datapath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {  // 如果打开成功
        QSqlQuery query(m_db);
        m_db.transaction();  // 开始事务

        // 使用占位符绑定的SQL语句
        query.prepare("INSERT INTO TEMP (DISTRICT, RECODEDATE, MINTEMP, MAXTEMP) VALUES (?, ?, ?, ?)");

        qint64 total = CountTxtRow(file);  // 获取文件的总行数
        file.seek(0);  // 重置文件指针到文件开头
        qint64 loaded = 0;

        QVector<QVariantList> batchData(4);  // 存储当前批量插入的数据
        int batchSize = 1000;  // 批量插入的大小

        QByteArray allData = file.readAll();  // 一次性读取所有文件内容
        file.close();
        QTextStream out(allData);
        while(!out.atEnd())
        {
            QString line=out.readLine();
            QStringList List = line.split('\t', Qt::SkipEmptyParts);

            if (List.size() < 4) continue;  // 忽略不完整的行

            QString district = List[0];
            qint64 dataEpoch = parseDateTime(List[1]);
            int minTemp = List[2].toInt();
            int maxTemp = List[3].toInt();

            if (district.isEmpty() || dataEpoch==-1) continue;

            // 将当前行数据添加到批量数据列表中
            batchData[0].push_back(district);
            batchData[1].push_back(dataEpoch);
            batchData[2].push_back(minTemp);
            batchData[3].push_back(maxTemp);

            loaded++;

            // 如果批量数据达到设定的大小，就执行批量插入
            if (batchData[0].size() >= batchSize) {
                for (int i = 0; i < 4; ++i)
                    query.addBindValue(batchData[i]);

                // 使用 execBatch() 执行批量插入
                if (!query.execBatch()) {
                    emit error(query.lastError().text());
                    qDebug() << query.lastError().text();
                    m_db.rollback();
                    return;
                }

                for (int i = 0; i < 4; ++i)
                    batchData[i].clear();

                emit LoadProgress(total, loaded);  // 更新进度
            }
        }

        // 处理剩余数据（如果有的话）
        if (!batchData[0].isEmpty()) {
            for (int i = 0; i < 4; ++i)
                query.addBindValue(batchData[i]);
            if (!query.execBatch()) {
                emit error(query.lastError().text());
                m_db.rollback();
                return;
            }
        }

        m_db.commit();  // 提交剩余的批量数据
        emit LoadProgress(loaded, loaded);  // 最终进度更新
        emit workOK(); //正常执行
    }
    else {
        emit error("Failed to open file");
    }
}


//传递一个保存温度数据的xlsx文件路径将其批量插入数据库中。为了插入高效，使用事务和批量插入exebatch
void TData::InSertRecords()
{
    QFileInfo info(datapath);
    if(info.isFile())
    {
        if(info.suffix()=="txt")
            InsertRecordsFromTxT();
    }
    emit workFinished();
}

//传递地区和开始时间和结束时间检索返回数据
std::vector<Record> TData::SelectTemp(const QString &district, const qint64 &start, const qint64 &end)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT RECODEDATE, MINTEMP, MAXTEMP FROM TEMP WHERE DISTRICT=? AND RECODEDATE>=? AND RECODEDATE<=? ORDER BY RECODEDATE ASC");
    query.addBindValue(district);
    query.addBindValue(start);
    query.addBindValue(end);

    std::vector<Record>ret;

    if(!query.exec())
    {
        emit error(query.lastError().text());
        return ret;
    }else
    {
        while(query.next())
        {
            Record cord;
            //cord.time=query.value(0).toLongLong();
            cord.mintemp=query.value(1).toDouble();
            cord.maxtemp=query.value(2).toDouble();
            ret.emplace_back(cord);
        }
        return ret;
    }
}

//查询指定时间段月平均温度
std::vector<Record> TData::SelectAvgTemp(const QString &district, const qint64 &start, const qint64 &end)
{
    std::vector<Record>result;
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT
            strftime('%Y-%m', datetime(RECODEDATE, 'unixepoch')) AS month,
            AVG(MINTEMP) AS avgmin,
            AVG(MAXTEMP) AS avgmax
        FROM
            TEMP
        WHERE
            DISTRICT = ?
            AND RECODEDATE >= ?
            AND RECODEDATE <= ?
        GROUP BY
            strftime('%Y-%m', datetime(RECODEDATE, 'unixepoch'))
        ORDER BY
            month ASC
    )");

    query.addBindValue(district);
    query.addBindValue(start);
    query.addBindValue(end);

    if(query.exec())
    {
        while(query.next())
        {
            QString time=query.value(0).toString();
            double avgmin=query.value(1).toDouble();
            double avgmax=query.value(2).toDouble();

            avgmin=std::round(avgmin*100)/100.0;
            avgmax=std::round(avgmax*100)/100.0;
            result.emplace_back(time,avgmin,avgmax);
        }
    }else{
        qDebug()<<query.lastError().text();
        emit error(query.lastError().text());
    }
    return result;
}

void TData::setDataPath(const QString &path)
{
    datapath=path;
}

//大体估算行数
qint64 TData::CountTxtRow(QFile &file)
{
    qint64 filesize=file.size();

    QTextStream stream(&file);
    QString str=stream.readLine();
    qint64 rowSize=str.toUtf8().size();

    qint64 SumRow=(filesize/rowSize);
    return SumRow;
}


int64_t TData::parseDateTime(const QString &dateStr)
{
    QStringList parts = dateStr.split(" ", Qt::SkipEmptyParts);
    if (parts.size() != 2)
        return -1;  // 如果格式不对，返回无效的 QDateTime

    QStringList dateParts = parts[0].split("-", Qt::SkipEmptyParts);  // 日期部分 "yyyy-MM-dd"
    QStringList timeParts = parts[1].split(":", Qt::SkipEmptyParts);  // 时间部分 "HH:mm:ss"

    if (dateParts.size() != 3 || timeParts.size() != 3)
        return -1;  // 如果格式不对，返回无效的 QDateTime

    int year = dateParts[0].toInt();
    int month = dateParts[1].toInt();
    int day = dateParts[2].toInt();
    int hour = timeParts[0].toInt();
    int minute = timeParts[1].toInt();
    int second = timeParts[2].toInt();

    QDateTime dateTime(QDate(year, month, day), QTime(hour, minute, second));
    return dateTime.isValid() ? dateTime.toSecsSinceEpoch() : -1;  // 如果无效，则返回无效的 QDateTime
}
