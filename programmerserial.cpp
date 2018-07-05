#include "programmerserial.h"
#include <QtCore/QtGlobal>
#include <synchapi.h>
#include <QDateTime>
#include <QScrollBar>
#include <QMessageBox>
#include <QLabel>
#include <QtSerialPort/QSerialPort>
#include <QFileDialog>
#include <QTextStream>
#include <qtimer.h>


programmerSerial::programmerSerial(QWidget *parent) : QWidget(parent)
{
    serial = new QSerialPort(this);
    timer = new QTimer(this);
}

QString programmerSerial::ShowHex(QByteArray str)
{
    QDataStream out(&str,QIODevice::ReadWrite); 
    QString buf;
    while(!out.atEnd())
    {
        qint8 outChar = 0;
        out >> outChar; 
        QString str = QString("%1").arg(outChar&0xFF,2,16,QLatin1Char('0')).toUpper() + QString(" "); 

        buf += str;
    }
    return buf;
}

char programmerSerial::ConvertHexChar(char ch)
{
    if ((ch >= '0') && (ch <= '9'))
    {
        return ch-0x30;
    }
    else if ((ch >= 'A') && (ch <= 'F'))
    {
        return ch-'A'+10;
    }
    else if ((ch >= 'a') && (ch <= 'f'))
    {
        return ch-'a'+10;
    }
    else
    {
        return (-1);
    }
}

//将字符型进制转化为16进制的字节数组
QByteArray programmerSerial::QString2Hex(QString str)
{
    QByteArray senddata;
    int hexdata,lowhexdata;
    int hexdatalen = 0;
    int len = str.length();
    char lstr,hstr;

    senddata.resize(len/2);

    for(int i=0; i<len; i++)
    {
        hstr=str[i].toLatin1();   //字符型
        if(hstr == ' ')
        {
            continue;
        }

        i++;
        lstr = str[i].toLatin1();
        if(lstr == ' ' || i >= len) // 保证单字符或最后一个是单字符的情况下发送正确。
        {
            hexdata = 0;
            lowhexdata = ConvertHexChar(hstr);
        }
        else
        {
            hexdata = ConvertHexChar(hstr);
            lowhexdata = ConvertHexChar(lstr);
        }

        if((hexdata == -1) || (lowhexdata == -1)) // 输入不合法
        {
            senddata.resize(hexdatalen);
            return senddata;
        }
        else
        {
            hexdata = hexdata*16 + lowhexdata;
        }
        senddata[hexdatalen] = (char)hexdata;
        hexdatalen++;
    }
    senddata.resize(hexdatalen);
    return senddata;
}


