#ifndef PROGRAMMERSERIAL_H
#define PROGRAMMERSERIAL_H

#include <QObject>
#include <QWidget>
#include <QtCore/QtGlobal>
#include <synchapi.h>
#include <QDateTime>
#include <QScrollBar>
#include <QMessageBox>
#include <QLabel>
#include <QtSerialPort/QSerialPort>
#include <QFileDialog>
#include <QTextStream>

class programmerSerial : public QWidget
{
    Q_OBJECT
public:
    explicit programmerSerial(QWidget *parent = nullptr);
    QSerialPort *serial;
    QTimer *timer;
    int recvCnt = 0;
    int sendCnt = 0;
    int sendTime;

    QString ShowHex(QByteArray str);
    char ConvertHexChar(char ch);
    QByteArray QString2Hex(QString str);

signals:

public slots:
};

#endif // PROGRAMMERSERIAL_H
