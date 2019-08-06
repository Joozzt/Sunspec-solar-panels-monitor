#include "options.h"
#include "ui_options.h"

Options::Options(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Options)
{
    ui->setupUi(this);
}

Options::~Options()
{
    delete ui;
}

void Options::showEvent(QShowEvent*)
{
    ui->InverterAddress->setText(InverterIP);
    ui->YoulessAddress->setText(YoulessIP);
    ui->pvo_id->setText(PVO_systemid);
    ui->pvo_key->setText(PVO_apikey);
    ui->wundergroundid->setText(WundergroundID);

}

void Options::on_buttonBox_accepted()
{
    if(InverterIP!=ui->InverterAddress->text() || YoulessIP!=ui->YoulessAddress->text())
    {
        InverterIP=ui->InverterAddress->text();
        YoulessIP=ui->YoulessAddress->text();
        emit newIPs();
    }
    PVO_systemid=ui->pvo_id->text();
    PVO_apikey=ui->pvo_key->text();
    WundergroundID=ui->wundergroundid->text();
}
