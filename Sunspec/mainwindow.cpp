#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include <QAbstractSocket>
#include <QtNetwork>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>


#define YOULESS_IP "192.168.178.23:80"
#define INVERTER_IP "192.168.178.10:502"

#define PVOID "123456"
#define PVOKEY "0123456789abcdef"

#define WUNDERGROUNDSTATION ""

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    opt= new Options;
    connect(ui->actionOptions,SIGNAL(triggered()),this,SLOT(selectOptions()));
    connect(opt,SIGNAL(newIPs()),this,SLOT(newIPs()));
        m_pTcpSocket = new QTcpSocket(this);

    m_pTcpSocket->setSocketOption(QAbstractSocket::KeepAliveOption,1);
    connect(m_pTcpSocket,SIGNAL(readyRead()),SLOT(readSocketData()),Qt::UniqueConnection);
    connect(m_pTcpSocket,SIGNAL(disconnected()),SLOT(onConnectionTerminated()),Qt::UniqueConnection);
    connect(m_pTcpSocket,SIGNAL(connected()),SLOT(onConnectionEstablished()),Qt::UniqueConnection);

    manager = new QNetworkAccessManager(this);
//    AddNetWorkProxy( manager );
    connect(manager, SIGNAL(sslErrors(QNetworkReply*, QList<QSslError>)),this, SLOT(sslErrors(QNetworkReply*, QList<QSslError>)));
    connect(manager, SIGNAL(finished(QNetworkReply*)),this, SLOT(replyFinished(QNetworkReply*)));

    settings= new QSettings("OMT","Solaredge modbus monitor");
    lastdate=settings->value("Last Date","").toString();
    lastenergy=settings->value("Last Energy",0).toUInt();
    lastyoulessenergy=settings->value("Last Youless Energy",0).toUInt();
    opt->YoulessIP= settings->value("YoulessIP",YOULESS_IP).toString();
    opt->InverterIP= settings->value("InverterIP",INVERTER_IP).toString();
    opt->PVO_systemid = settings->value("PVO id",PVOID).toString();
    opt->PVO_apikey = settings->value("PVO key",PVOKEY).toString();
    opt->WundergroundID = settings->value("WundergroundID",WUNDERGROUNDSTATION).toString();

    for(int i=0;i<YOULESSBUFFERSIZE;i++)
    {
        youlessDelayBuffer[i]=lastyoulessenergy;
    }
    youlessenergy=lastinterenergy=lastinteryoulessenergy=0;
    QDateTime dt=QDateTime::currentDateTime();
    lastintrahour=dt.time().minute()/5;
    maxpower=avgpowerf=avgdcpowerf=avgpowercnt=maxdcpower=0;
    minpower=mindcpower=mindcvoltagef=1000000000;
    sdp=0;
    sunspecrp=youlesswp=0;
    lastradiation=0;
    lastradiationtime=13*60;//-1;           //not valid yet, take only 1 last time of table
    WundergroundDelay=0;
    requestingdata=false;
    polltimer=new QTimer(this);
    connect(polltimer,SIGNAL(timeout()),this,SLOT(mytimer()));
    transactionnr=0;
}

MainWindow::~MainWindow()
{
    m_pTcpSocket->disconnectFromHost();
    settings->setValue("Last Date",lastdate);
    settings->setValue("Last Energy",lastenergy);
    settings->setValue("Last Youless Energy",lastyoulessenergy);
    settings->setValue("InverterIP",opt->InverterIP);
    settings->setValue("YoulessIP",opt->YoulessIP);
    settings->setValue("PVO id",opt->PVO_systemid);
    settings->setValue("PVO key",opt->PVO_apikey);
    settings->setValue("WundergroundID",opt->WundergroundID);
    delete ui;
}

void MainWindow::sslErrors(QNetworkReply * reply, QList<QSslError> errors)
{
    QString text=ui->uploadlabel->text();
    for(int i=0;i<errors.size();i++)
    {
        qDebug()<<"SSLError:"<<errors.at(i).errorString();
        text.append(QString::asprintf("SSLError:%s\n",errors.at(i).errorString().toLatin1().data()));
    }
    ui->uploadlabel->setText(text);
    reply->ignoreSslErrors();
}

void MainWindow::replyFinished(QNetworkReply* reply)
{
    if(reply==WundergroundReply)
    {
        if(reply->error()==QNetworkReply::NoError)
        {
            QString url=QString::asprintf("https://www.pvoutput.org/service/r2/addbatchstatus.jsp?key=%s&sid=%s&data=",opt->PVO_apikey.toLatin1().data(),opt->PVO_systemid.toLatin1().data());
            QString batchdata;
            bool havenewdata=false;
            float radiation;
            int hr,min;
            QByteArray data=reply->readAll();
            data.append('\0');
            char* text=data.data();
            char * p=strstr(text,"history-table desktop-table");
            if(p)p=strstr(p,"<tbody");
            if(p)
            {
                p=strstr(p,"<tr");
                while(p)
                {
                    p+=3;
                    char* pnext=strstr(p,"<tr");
                    if(pnext)
                    {
                        *pnext=0;
                    }
                    char *m=strstr(p,"AM");
                    bool pmtime=false;
                    if(!m)
                    {
                        pmtime=true;
                        m=strstr(p,"PM");
                    }
                    if(m)m=strstr(m-10,">");
                    if(m)
                    {//found a time
                        m++;
                        sscanf_s(m,"%d:%d",&hr,&min);
                        if(hr>=12)hr-=12;
                        if(pmtime)hr+=12;
                    }
                    char* solar=strstr(p,"w/m")-24;
                    if(solar)solar=strstr(solar,">");
                    if(solar)
                    {
                        radiation=atof(solar+1);
//                        qDebug()<<radiation;
                    }
                    if(solar&&m)
                    {
                        int radiationtime=hr*60+min;
                        qDebug()<<hr<<min<<radiation;
                        if(lastradiationtime>0)
                        {
                            if(radiationtime>lastradiationtime || (lastradiationtime-radiationtime)>15*60)      //only if new time later than last (or just after midnight)
                            {
                                havenewdata=true;
                                QDateTime dt=QDateTime::currentDateTime();
                                QString date=dt.toString("yyyyMMdd");
                                batchdata+=date+",";
                                batchdata+=QString::asprintf("%02d:%02d,0,,,0,,,,,,,,%f;",hr,min,lastradiation);
                                lastradiation=radiation;
                                lastradiationtime=radiationtime;
                            }
                        }
                        else
                        {
                            havenewdata=true;
                        }
                    }
                    p=pnext;
                }
            }
//only use last entry of the table, and only if time is later than previous time
            if(havenewdata)
            {
                if(lastradiationtime<0)lastradiationtime=hr*60+min;
                qDebug()<<"url:"<<url+batchdata;
                if(batchdata.size())pvoutputReply=manager->get(QNetworkRequest(QUrl(url+batchdata)));
#if 0
                int radiationtime=hr*60+min;
                qDebug()<<hr<<min<<radiation;
                if(radiationtime>lastradiationtime || (lastradiationtime-radiationtime)>12*60)      //only if new time later than last (or just after midnight)
                {
                    lastradiation=radiation;
                    lastradiationtime=radiationtime;
                    ui->youless_label->clear();
                    ui->youless_label->setText(QString::asprintf("solar radiation:%f",lastradiation));
                }
#endif
            }
        }
        else {
            qDebug()<<"WundergroundReply reply error:"<<reply->errorString();
        }
    }
    else if(!opt->YoulessIP.isEmpty() && reply->url().toString().contains(opt->YoulessIP))
    {
        if(reply->error()==QNetworkReply::NoError)
        {
            QJsonDocument loadDoc( QJsonDocument::fromJson(reply->readAll()));
            const QJsonObject & json = loadDoc.object();
            int yenergy=json["cnt"].toString().replace(",","").toInt();
            youlessDelayBuffer[youlesswp++]=yenergy;
            youlesswp%=YOULESSBUFFERSIZE;
            youlesspower=(int)json["pwr"].toDouble();

            double youless_energy=0;
            int yrp=youlesswp-YOULESSDELAY-2;
            if(yrp<0)yrp+=YOULESSBUFFERSIZE;
            for(int i=0;i<YOULESSDELAY;i++)
            {
                youless_energy+=(double)youlessDelayBuffer[yrp++];
                yrp%=YOULESSBUFFERSIZE;
            }
            youless_energy/=(double)YOULESSDELAY;
            youlessenergy=youless_energy;

            qDebug()<<"youless energy:"<<youlessenergy<<"pwr:"<<youlesspower;
            ui->youless_label->clear();
            ui->youless_label->setText(QString::asprintf("cnt:%d interpolated:%d power:%d",yenergy,youlessenergy,youlesspower));
        }
        else {
            qDebug()<<"youless reply error:"<<reply->errorString();
        }

    }
    else
    {
        QString text=ui->uploadlabel->text();
        if(reply->error()==QNetworkReply::NoError)
        {
            text.append("Upload OK\n");
            ui->uploadlabel->setText(text);
        }
        else {
            text.append(QString::asprintf("error:%s returned:%s\n",reply->errorString().toLatin1().data(),reply->readAll().data()));
            qDebug()<<"pvoutput reply error:"<<reply->errorString()<<reply->readAll();
            ui->uploadlabel->setText(text);
        }
    }
    reply->deleteLater();
}

void MainWindow::selectOptions()
{
    opt->show();
}

void MainWindow::newIPs()
{

}

void MainWindow::requestData()
{
    unsigned char data[]={0,0,0,0,0,6,0x01,0x03,00,71,0,38};
    data[0]=(transactionnr>>8)&0xff;
    data[1]=transactionnr&0xff;
    requestingdata=true;
    int written=m_pTcpSocket->write((char*)data, 12);
    qDebug()<<"written:"<<written;
    ui->label->clear();
    QDateTime dt=QDateTime::currentDateTime();
    QString time=dt.toString("hh:mm:ss");
    ui->label->setText(time+QString::asprintf("\nwritten:%d",written));
    transactionnr++;
}

void MainWindow::mytimer()
{
    if(!opt->WundergroundID.isEmpty())
    {
        if(WundergroundDelay<=0)
        {
            WundergroundDelay=3600;     //set to a value, changed later
            QDateTime dt=QDateTime::currentDateTime();
            QString date=dt.toString("yyyy-MM-dd");
            QString url;
            url.sprintf("https://www.wunderground.com/dashboard/pws/%s/table/%s/%s/daily",opt->WundergroundID.toLatin1().data(),date.toLatin1().data(),date.toLatin1().data());
            qDebug()<<url;
            WundergroundReply=manager->get(QNetworkRequest(QUrl(url)));
        }
        WundergroundDelay--;
    }
    if(!opt->YoulessIP.isEmpty())
    {
        QString url="http://" +opt->YoulessIP ;
        url+="//a?f=j";
        youlessReply=manager->get(QNetworkRequest(QUrl(url)));
    }
    if(!opt->InverterIP.isEmpty())
    {
        if(!(QAbstractSocket::ConnectedState == m_pTcpSocket->state()))
        {
            QStringList ip=opt->InverterIP.split(':');
            m_pTcpSocket->connectToHost( ip.at(0), ip.size()==2 ? ip.at(1).toInt():502, QIODevice::ReadWrite);
            requestingdata=false;
            return;
        }
        if(requestingdata)
        {
            m_pTcpSocket->disconnectFromHost();
            requestingdata=false;
            return;
        }
        if(m_pTcpSocket->state()==QAbstractSocket::ConnectedState)
        {
            requestData();
        }
    }
}

void MainWindow::onConnectionEstablished()
{
    qDebug()<<"connection established";
}

void MainWindow::onConnectionTerminated()
{
    qDebug()<<"connection terminated";
}

float MainWindow::read16bitAndScale(char* buff, int n, int scalereg)
{
    int index=n*2;
    int scaleoffset=scalereg*2;
    buff+=index;
    int result=(*buff<<8)|(*(buff+1)&0xff);
    short sf=(*(buff+scaleoffset)<<8)|(*(buff+scaleoffset+1)&0xff);
    return (float)result*pow(10.0,sf);
}

void MainWindow::readSocketData()
{
    requestingdata=false;
    while(m_pTcpSocket->bytesAvailable())
    {
        QByteArray receivedData = sunspecData[sunspecrp];
        sunspecData[sunspecrp]=m_pTcpSocket->readAll();
        float directpowerf=read16bitAndScale(sunspecData[sunspecrp].data()+9,12);
        sunspecrp++;
        if(sunspecrp>=SUNSPECDATADELAY)sunspecrp=0;
        QString text=ui->label->text();
        text.append(QString::asprintf("\nAC Power now:%.1f",directpowerf));

        if(receivedData.isEmpty())return;

        char *data=receivedData.data()+9;

        qDebug()<<"read:"<<receivedData.length();
        if(receivedData.length()<10)return;

        float actotalcurrent=read16bitAndScale(data,0,4);
        float acphaseAcurrent=read16bitAndScale(data,1,3);
        float acphaseBcurrent=read16bitAndScale(data,2,2);
        float acphaseCcurrent=read16bitAndScale(data,3,1);
        float acphaseAVoltage=read16bitAndScale(data,8,3);
        float acphaseBVoltage=read16bitAndScale(data,9,2);
        float acphaseCVoltage=read16bitAndScale(data,10,1);
        float powerf=read16bitAndScale(data,12);
        float dccurrentf=read16bitAndScale(data,25);
        float dcvoltagef=read16bitAndScale(data,27);
        float dcpowerf=read16bitAndScale(data,29);
        float hstemp=read16bitAndScale(data,32,3);
        float a_power=read16bitAndScale(data,16);
        float r_power=read16bitAndScale(data,18);
        float power_factor=read16bitAndScale(data,20);
        float calc_power=a_power*power_factor*-0.01;//a_power>r_power? powf(a_power*a_power-r_power*r_power,0.5):0.0;

        unsigned int energy=(receivedData.at(44+9)<<24)&0xff000000;
        energy+=(receivedData.at(45+9)<<16)&0xff0000;
        energy+=(receivedData.at(46+9)<<8)&0xff00;
        energy+=(receivedData.at(47+9))&0xff;

        qDebug()<<"power:"<<powerf<<"energy:"<<energy<<"voltage:"<<dcvoltagef<<"DC current:"<<dccurrentf;
        if(powerf>maxpower)
        {
            maxpower=powerf;
            maxdcpower=dcpowerf;
        }
        if(powerf<minpower)
        {
            minpower=powerf;
            mindcpower=dcpowerf;
        }
        avgpowerf+=powerf;
        avgdcpowerf+=(double)dccurrentf*dcvoltagef;
        avgpowercnt++;

        if(dcvoltagef<mindcvoltagef)mindcvoltagef=dcvoltagef;

        double efficiency=avgdcpowerf>50.0? 1000.0*avgpowerf/avgdcpowerf:985.0;

        text.append(QString::asprintf("\nCalculated AC Power:%.1f power-factor:%.1f efficiency:%.4f",calc_power,power_factor,efficiency));
        text.append(QString::asprintf("\nAC Power:%.1f DC Current:%.3f DC Voltage:%.2f, heat sink temp:%.1f",powerf,dccurrentf,dcvoltagef,hstemp));
        text.append(QString::asprintf("\nDC Power:%.2f Calculated DC power:%.2f Min DC Voltage:%f",dcpowerf,(dccurrentf*dcvoltagef),mindcvoltagef));
        text.append(QString::asprintf("\nYouless energy:%d",youlessenergy));
        text.append(QString::asprintf("\nSolar radiation:%f",lastradiation));
        qDebug()<<"maxpower:"<<maxpower<<"minpower:"<<minpower<<"avgpower:"<<(avgpowerf/avgpowercnt)<<"avg DC power:"<<(avgdcpowerf/avgpowercnt);
        qDebug()<<"maxdcpower:"<<maxdcpower<<"mindcpower:"<<mindcpower;
        qDebug()<<"efficiency:"<<efficiency;
        text.append(QString::asprintf("\nmaxpower:%d minpower:%d avgpower:%.1f",maxpower,minpower,(avgpowerf/avgpowercnt)));
        text.append(QString::asprintf("\nmaxdcpower:%d mindcpower:%d",maxdcpower,mindcpower));
        if(!lastinteryoulessenergy)lastinteryoulessenergy=youlessenergy;
        if(!lastinterenergy)lastinterenergy=energy;
        qDebug()<<"avg youless power:"<<QString::number((youlessenergy-lastinteryoulessenergy)*12);
        qDebug()<<"avg solar power:"<<QString::number((energy-lastinterenergy)*12);
        qDebug()<<"avg used power:"<<QString::number((youlessenergy-lastinteryoulessenergy+(int)energy-lastinterenergy)*12);
        qDebug()<<"totalcnt:"<<(youlessenergy+energy);
        ui->label->setText(text);
        QString url=QString::asprintf("https://www.pvoutput.org/service/r2/addstatus.jsp?key=%s&sid=%s&c1=1",opt->PVO_apikey.toLatin1().data(),opt->PVO_systemid.toLatin1().data());
        if(!lastenergy)lastenergy=energy;
        if(!lastyoulessenergy)lastyoulessenergy=youlessenergy;
        QDateTime dt=QDateTime::currentDateTime();
        QString date=dt.toString("yyyyMMdd");
        QString time=dt.toString("hh:mm");
        QString times=dt.toString("hh:mm:ss");
        QFile fn(date+".csv");
        fn.open(QIODevice::WriteOnly|QIODevice::Append);
        if(date.compare(lastdate))
        {
            qDebug()<<"date:"<<date<<"Lastdate:"<<lastdate;
            lastenergy=energy;
            lastyoulessenergy=youlessenergy;
            lastdate=date;
            settings->setValue("Last Date",lastdate);
            settings->setValue("Last Energy",lastenergy);
            settings->setValue("Last Youless Energy",lastyoulessenergy);
            fn.write("inverter power,inverter energy,youless energy,youless power,DC power, DC Current, DC Voltage, AC total current,AC Phase A current,AC Phase B current,AC Phase C current,AC Phase A voltage,AC Phase B voltage,AC Phase C voltage,Powerfactor,time\n");
        }
        int intrahour=dt.time().minute()/5;
//save data to csv file
        fn.write(QString::asprintf("%d,%d,%d,%d,",(int)powerf,(energy-lastenergy),((int)youlessenergy-lastyoulessenergy),youlesspower).toLatin1());
        fn.write(QString::asprintf("%f,%f,%f,",dcpowerf,dccurrentf,dcvoltagef).toLatin1());
        fn.write(QString::asprintf("%f,%f,%f,%f,",actotalcurrent,acphaseAcurrent,acphaseBcurrent,acphaseCcurrent).toLatin1());
        fn.write(QString::asprintf("%f,%f,%f,%f,",acphaseAVoltage,acphaseBVoltage,acphaseCVoltage,power_factor).toLatin1());
        fn.write(QString::asprintf("%s\n",times.toLatin1().data()).toLatin1());
        fn.close();
        if(intrahour != lastintrahour)
        {//upload
            WundergroundDelay=210;                     //get solar radiation in 3.5 minutes
            lastintrahour=intrahour;
            if(!opt->PVO_systemid.isEmpty())
            {
                url+="&d=" + date;
                url+="&t=" + time;
                url+="&v1=" + QString::number(energy);
                url+="&v2=" + QString::number((int)maxpower);
                url+="&v3=" + QString::number(((int)youlessenergy)+energy);
                url+="&v6=" + QString::number(mindcvoltagef);
                url+="&v7=" + QString::number((int)maxpower);
                url+="&v8=" + QString::number((int)minpower);
                url+="&v9=" + QString::number((int)(avgpowerf/avgpowercnt));
                url+="&v11=" + QString::number(efficiency);
                url+="&v12=" + QString::number(lastradiation);
    //            url+="&v9=" + QString::number((energy-lastenergy)*12);
                qDebug()<<"url:"<<url;
                ui->uploadlabel->setText(url+"\n");
                pvoutputReply=manager->get(QNetworkRequest(QUrl(url)));
            }
            maxpower=maxdcpower=avgpowerf=avgdcpowerf=avgpowercnt=0;
            minpower=mindcpower=mindcvoltagef=1000000000;
            lastinteryoulessenergy=youlessenergy;
            lastinterenergy=energy;
        }
    }
}

void MainWindow::on_checkBox_clicked(bool checked)
{
    if(checked)
    {
        polltimer->start(TIMERDELAY*1000);

    }
    else {
        polltimer->stop();
        m_pTcpSocket->disconnectFromHost();
    }
}
