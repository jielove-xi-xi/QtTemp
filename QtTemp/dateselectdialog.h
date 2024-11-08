#ifndef DATESELECTDIALOG_H
#define DATESELECTDIALOG_H

#include <QDialog>

namespace Ui {
class DateSelectDialog;
}

class DateSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DateSelectDialog(QWidget *parent = nullptr);
    ~DateSelectDialog();
    std::pair<QDateTime,QDateTime> GetTime();
private:
    Ui::DateSelectDialog *ui;
};

#endif // DATESELECTDIALOG_H
