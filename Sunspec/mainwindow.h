#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork>
#include <QTcpServer>
#include <QTcpSocket>

#include "options.h"

#define TIMERDELAY 1
#define YOULESSDELAY (10/TIMERDELAY)
#define YOULESSBUFFERSIZE (2*YOULESSDELAY+2)
#define SUNSPECDATADELAY (18/TIMERDELAY + YOULESSDELAY)

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

private slots:

    void selectOptions();
    void newIPs();

    void on_checkBox_clicked(bool checked);

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QTcpSocket*   m_pTcpSocket;
    Options * opt;

    QTimer * polltimer;
    int transactionnr;
    QNetworkRequest uploadReq;

    void requestData();
    QString lastdate;
    unsigned int lastenergy;
    int youlessenergy,lastinteryoulessenergy,lastinterenergy,lastyoulessenergy;
    int maxpower,minpower,avgpowercnt,maxdcpower,mindcpower,youlesspower;
    float mindcvoltagef;
    double avgpowerf,avgdcpowerf;
    int sdp;
    int lastintrahour;
    QSettings *settings;
    QNetworkAccessManager* manager;
    bool requestingdata;
    QNetworkReply * pvoutputReply,*youlessReply;
    QByteArray sunspecData[SUNSPECDATADELAY];
    int sunspecrp;
    int youlessDelayBuffer[YOULESSBUFFERSIZE];
    int youlesswp;

    float read16bitAndScale(char *buff, int n, int scalereg=1);     //default scalereg=next one

public slots:

    void mytimer();
    void readSocketData();
    void onConnectionEstablished();

    void onConnectionTerminated();

    void sslErrors(QNetworkReply *reply, QList<QSslError> errors);
    void replyFinished(QNetworkReply *reply);

};

#endif // MAINWINDOW_H
