#include "dateselectdialog.h"
#include "ui_dateselectdialog.h"

DateSelectDialog::DateSelectDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DateSelectDialog)
{
    ui->setupUi(this);
    ui->startTimeEdit->setDateTime(QDateTime::currentDateTime());
    ui->endTimeEdit->setDateTime(QDateTime::currentDateTime());
}

DateSelectDialog::~DateSelectDialog()
{
    delete ui;
}

std::pair<QDateTime,QDateTime> DateSelectDialog::GetTime()
{
    return {ui->startTimeEdit->dateTime(),ui->endTimeEdit->dateTime()};
}
