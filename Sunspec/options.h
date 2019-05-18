#ifndef OPTIONS_H
#define OPTIONS_H

#include <QDialog>

namespace Ui {
class Options;
}

class Options : public QDialog
{
    Q_OBJECT

public:
    explicit Options(QWidget *parent = nullptr);
    ~Options();

    QString YoulessIP,InverterIP;
    QString PVO_systemid,PVO_apikey;

private slots:
    void on_buttonBox_accepted();

private:
    Ui::Options *ui;
    void showEvent(QShowEvent*);
//    void hideEvent(QHideEvent*);
//    void closeEvent(QCloseEvent*);
//    void resizeEvent(QResizeEvent *ev);
signals:
    void newIPs();
};

#endif // OPTIONS_H
