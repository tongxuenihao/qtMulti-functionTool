#include "mainwindow.h"
#include "ui_mainwindow.h"
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
#include<QSerialPortInfo>
#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QProcess>
#include <QtEndian>
#include<QDebug>
#include <QUdpSocket>
#include<QtNetwork/QTcpSocket>
#include<QtNetwork/QTcpServer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QPalette p = palette();
    p.setColor(QPalette::Base, Qt::black);
    p.setColor(QPalette::Text, Qt::green);
    setPalette(p);

    programmerserial = new programmerSerial;  //串口类
    mSocket = new QUdpSocket();
    mTcpServer = new QTcpServer();
    mTcpServerSocket = new QTcpSocket();
    mTcpClientSocket = new QTcpSocket();
    operateMenuInit();        //操作菜单初始化
    settingMenuInit();        //设置菜单初始化
    programmerSerialInit();   //串口部分初始化
    programmerUdpInit();      //UDP部分初始化
    programmerTcpInit();      //TCP部分初始化
}

MainWindow::~MainWindow()
{
    delete ui;
    delete programmerserial;
    delete mSocket;
    delete mTcpServer;
    delete mTcpServerSocket;
}

/*操作菜单*/
void MainWindow::operateMenuInit()
{
    ui->action_4->setEnabled(false);
    ui->actionudp->setEnabled(false);
    ui->actiontcp->setEnabled(false);
    connect(ui->action, &QAction::triggered, this, &MainWindow::saveAs);
    connect(ui->action_2, &QAction::triggered, this, &MainWindow::windowClear);
    connect(ui->action_4, &QAction::triggered, this, &MainWindow::serialDisconnect);
    connect(ui->action_5, &QAction::triggered, this, &MainWindow::programExit);
}

void MainWindow::serialDisconnect()
{
    if (programmerserial->serial->isOpen())
    {
        programmerserial->serial->close();
        showStatusMessage(tr(""));
        ui->pushButton_6->setText("打开串口");
        return;
    }
}

void MainWindow::programExit()
{
    exit(0);
}


/*设置菜单*/
void MainWindow::settingMenuInit()
{
    ui->action_6->setEnabled(true);
    ui->action_7->setEnabled(false);
    connect(ui->action_6, &QAction::triggered, this, &MainWindow::settingHide);
    connect(ui->action_7, &QAction::triggered, this, &MainWindow::settingOpen);
    //connect(ui->action_4, &QAction::triggered, this, &MainWindow::serialDisconnect);
    //connect(ui->action_5, &QAction::triggered, this, &MainWindow::programExit);
}

void MainWindow::settingHide()
{
    ui->groupBox->hide();
    ui->action_6->setEnabled(false);
    ui->action_7->setEnabled(true);
}

void MainWindow::settingOpen()
{
    ui->groupBox->show();
    ui->action_6->setEnabled(true);
    ui->action_7->setEnabled(false);
}


/*************************串口部分接口*********************************/
void MainWindow::programmerSerialInit()
{
    ui->lineEdit_5->setEnabled(false);
    ui->lineEdit_5->setText(QString::number(programmerserial->recvCnt));
    ui->lineEdit_6->setEnabled(false);
    ui->lineEdit_6->setText(QString::number(programmerserial->sendCnt));
    getSerialInfo();  //获取可用的串口

    connect(programmerserial->serial, static_cast<void (QSerialPort::*)(QSerialPort::SerialPortError)>(&QSerialPort::error),
            this, &MainWindow::handleError);
    connect(ui->pushButton_6, &QPushButton::clicked, this, &MainWindow::OpenSerial);
    connect(programmerserial->serial, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::writeData);
    connect(ui->pushButton_8, &QPushButton::clicked, this, &MainWindow::saveAs);
    connect(ui->pushButton_9, &QPushButton::clicked, this, &MainWindow::windowClear);
    connect(programmerserial->timer,SIGNAL(timeout()),this,SLOT(dataSendBaseOnTimer()));   //定时发送
}


void MainWindow::getSerialInfo()
{
    foreach( const QSerialPortInfo &Info,QSerialPortInfo::availablePorts())//读取串口信息
    {
        QSerialPort serial;
        serial.setPort(Info);
        if( serial.open( QIODevice::ReadWrite) )
        {
            ui->comboBox->addItem( Info.portName() );
            serial.close();
        }
    }
}

void MainWindow::OpenSerial()
{
    programmerserial->serial->setPortName(ui->comboBox->currentText());
    programmerserial->serial->setBaudRate(ui->comboBox_2->currentText().toInt());
    programmerserial->serial->setDataBits(QSerialPort::Data8);

    if (programmerserial->serial->isOpen())
    {
        programmerserial->serial->close();
        showStatusMessage(tr(""));
        ui->pushButton_6->setText("打开串口");
        ui->action_4->setEnabled(false);
        return;
    }

    if (programmerserial->serial->open(QIODevice::ReadWrite)) {
        showStatusMessage(tr("Connected to %1 : %2, %3")
                          .arg(ui->comboBox->currentText()).arg(ui->comboBox_2->currentText()).arg(ui->comboBox_3->currentText()));
        ui->pushButton_6->setText("关闭串口");
        ui->action_4->setEnabled(true);
    } else {
        QMessageBox::critical(this, tr("Error"), programmerserial->serial->errorString());

        showStatusMessage(tr("Open error"));
    }
}

void MainWindow::readData()
{
    static QByteArray data; 
    data += programmerserial->serial->readAll();
    QString receiveMsg; 
    if(data.contains("\n"))
    {
        if(ui->checkBox_5->isChecked())
            receiveMsg = programmerserial->ShowHex(data);
        else
            receiveMsg = QString::fromLocal8Bit(data);

        ui->textBrowser->append(receiveMsg);
        data.clear();
    }
    if(ui->checkBox_7->isChecked())
    {
        programmerserial->recvCnt += data.count();
        QString s = QString::number(programmerserial->recvCnt);
        ui->lineEdit_5->setText(s);
    }
}

void MainWindow::writeData()
{
    if (!programmerserial->serial->isOpen())
    {
        QMessageBox::critical(this, tr("Error"), "请打开串口");
        return;
    }
    QString mString = ui->lineEdit->text();
    if(ui->checkBox->isChecked())
    {
        mString = programmerserial->QString2Hex(mString);
    }
    QByteArray temp= mString.toLatin1();
    if(ui->checkBox_2->isChecked())
    {
        QString tempTime = ui->lineEdit_2->text();
        programmerserial->sendTime = tempTime.toInt();
        if(programmerserial->sendTime == 0)
        {
            QMessageBox::critical(this, tr("Critical Error"), "请设置定时时间");
            return;
        }
        programmerserial->timer->start(programmerserial->sendTime);
    }
    else
    {
        if(programmerserial->timer->isActive())
        {
            programmerserial->timer->stop();
        }
        programmerserial->serial->write(temp.data(),temp.count());
    }

    if(ui->checkBox_7->isChecked())
    {
        programmerserial->sendCnt += temp.count();
        QString s = QString::number(programmerserial->sendCnt);
        ui->lineEdit_6->setText(s);
    }
}

void MainWindow::dataSendBaseOnTimer()
{
    if(ui->checkBox_2->isChecked())
    {
        QString mString = ui->lineEdit->text();
        if(ui->checkBox->isChecked())
        {
            mString = programmerserial->QString2Hex(mString);
        }
        QByteArray temp= mString.toLatin1();
        programmerserial->serial->write(temp.data(),temp.count());
    }
    else
    {
        programmerserial->timer->stop();
    }
}

void MainWindow::windowClear()
{
    ui->textBrowser->clear();
    ui->lineEdit->clear();
    ui->lineEdit_2->clear();
    ui->lineEdit_5->clear();
    ui->lineEdit_6->clear();
}


/*************************UDP部分接口*********************************/
void MainWindow::programmerUdpInit()
{
    //mSocket = new QUdpSocket();
    ui->groupBox_7->hide();
    connect(ui->pushButton_19, &QPushButton::clicked, this, &MainWindow::getLocalIp);
    connect(ui->pushButton_2, &QPushButton::clicked, this, &MainWindow::addMulticastInfo);
    connect(ui->pushButton_3, &QPushButton::clicked, this, &MainWindow::applyMulticastInfo);
    connect(ui->pushButton_4, &QPushButton::clicked, this, &MainWindow::udpInitDatagram);
    connect(ui->pushButton_20, &QPushButton::clicked, this, &MainWindow::udpSendData);
    connect(mSocket,SIGNAL(readyRead()),this,SLOT(udpRecvData()));
    connect(ui->pushButton_7, &QPushButton::clicked, this, &MainWindow::udpRecvSaveAs);
    connect(ui->pushButton_5, &QPushButton::clicked, this, &MainWindow::udpRecvClear);
    connect(ui->pushButton_10, &QPushButton::clicked, this, &MainWindow::udpSendClear);
    connect(ui->action_3, &QAction::triggered, this, &MainWindow::multicastInfoShow);
}

void MainWindow::getLocalIp()
{
    QString ipAddr;
    QList<QHostAddress> AddressList = QNetworkInterface::allAddresses();
    foreach(QHostAddress address, AddressList){
        if(address.protocol() == QAbstractSocket::IPv4Protocol &&
                address != QHostAddress::Null &&
                address != QHostAddress::LocalHost){
            if (address.toString().contains("127.0.")){
                continue;
            }
            ipAddr = address.toString();
            break;
        }
    }
    ui->lineEdit_3->setText(ipAddr);
}

void MainWindow::addMulticastInfo()
{
    if(!ui->checkBox_6->isChecked())
    {
        QMessageBox::critical(this, tr("Critical Error"), "请先选择组播");
        return;
    }
    ui->groupBox_7->show();
}

//0xE00000FF~0xEFFFFFFF
void MainWindow::applyMulticastInfo()
{
    QString mString = ui->pushButton_4->text();
    if(mString == "开启")
    {
        QMessageBox::critical(this, tr("Critical Error"), "请开启UDP");
        return;
    }

    QString multicastIp = ui->lineEdit_12->text();
    unsigned int multicastIpRange = inet_addr(multicastIp.toLocal8Bit().data());
    multicastIpRange = qFromBigEndian(multicastIpRange);
    if((multicastIpRange <= 0xE00000FF) || (multicastIpRange >= 0xEFFFFFFF))
    {
        QMessageBox::critical(this, tr("Critical Error"), "组播地址不合法，请查看帮助");
        return;
    } 

    QString multicastButton = ui->pushButton_3->text();
    if(multicastButton == "加入组播")
    {
        mSocket->joinMulticastGroup(QHostAddress(ui->lineEdit_12->text()));
        ui->pushButton_3->setText("退出组播");
    }
    else if(multicastButton == "退出组播")
    {
        mSocket->leaveMulticastGroup(QHostAddress(ui->lineEdit_12->text()));
        ui->pushButton_3->setText("加入组播");
    }
}

void MainWindow::udpInitDatagram()
{
    QString mString = ui->pushButton_4->text();
    if(mString == "关闭")
    {
        ui->checkBox_3->setEnabled(true);
        ui->checkBox_4->setEnabled(true);
        ui->checkBox_6->setEnabled(true);
        ui->pushButton_4->setText("开启");
        return;
    }
#if 0
    //本地IP
    QString ipStr = ui->lineEdit_3->text();
    if(ipStr == 0)
    {
        QMessageBox::critical(this, tr("Critical Error"), "请获取本地IP");
        return;
    }
#endif
    //端口号
    QString portStr = ui->lineEdit_4->text();
    if(portStr == 0)
    {
        QMessageBox::critical(this, tr("Critical Error"), "请输入本地端口号");
        return;
    }

    if((!ui->checkBox_3->isChecked()) && (!ui->checkBox_4->isChecked()) && (!ui->checkBox_6->isChecked()))
    {
        QMessageBox::critical(this, tr("Critical Error"), "请选择UDP类型");
        return; 
    }

    else if((ui->checkBox_3->isChecked()) && (!ui->checkBox_4->isChecked()) && (!ui->checkBox_6->isChecked()))
    {
        ui->checkBox_4->setEnabled(false);
        ui->checkBox_6->setEnabled(false);
    }

    else if((!ui->checkBox_3->isChecked()) && (ui->checkBox_4->isChecked()) && (!ui->checkBox_6->isChecked()))
    {
        QString ipStr = ui->lineEdit_7->text();
        if(ipStr == 0)
        {
            QMessageBox::critical(this, tr("Critical Error"), "请输入目的IP");
            return;
        }
        ui->checkBox_3->setEnabled(false);
        ui->checkBox_6->setEnabled(false);
    }

    else if((!ui->checkBox_3->isChecked()) && (!ui->checkBox_4->isChecked()) && (ui->checkBox_6->isChecked()))
    {
        ui->checkBox_3->setEnabled(false);
        ui->checkBox_4->setEnabled(false);
    }

    else
    {
        QMessageBox::critical(this, tr("Critical Error"), "只能选择一种UDP类型");
        return; 
    }

    mSocket->bind(QHostAddress::AnyIPv4,portStr.toInt());
    ui->pushButton_4->setText("关闭");
}

void MainWindow::udpSendData()
{
    QString mString = ui->pushButton_4->text();
    if(mString == "开启")
    {
        QMessageBox::critical(this, tr("Critical Error"), "请先开启");
        return; 
    }

    QString dataStr = ui->lineEdit_13->text();

    if(dataStr == 0)
    {
        QMessageBox::critical(this, tr("Critical Error"), "请输入数据");
        return; 
    }

    if(ui->checkBox_3->isChecked())
    {
        if(ui->checkBox_10->isChecked())
        {
            QByteArray temp = QString2Hex(dataStr);
            mSocket->writeDatagram(temp,QHostAddress::Broadcast,ui->lineEdit_4->text().toInt());
        }
        else
        {
            mSocket->writeDatagram(dataStr.toUtf8(),QHostAddress::Broadcast,ui->lineEdit_4->text().toInt());
        }

        if(ui->checkBox_9->isChecked())
        {
            ui->textBrowser_2->append("发送广播包：" + dataStr.toUtf8());
        }
    }

    else if(ui->checkBox_4->isChecked())
    {
        if(ui->checkBox_10->isChecked())
        {
            QByteArray temp = QString2Hex(dataStr);
            //QByteArray temp= mString.toLatin1();
            mSocket->writeDatagram(temp.data(),temp.size(),QHostAddress(ui->lineEdit_7->text()),ui->lineEdit_4->text().toInt());
        }
        else
        {
            mSocket->writeDatagram(dataStr.toUtf8(),QHostAddress(ui->lineEdit_7->text()),ui->lineEdit_4->text().toInt());
        }

        if(ui->checkBox_9->isChecked())
        {
            ui->textBrowser_2->append("发送单播包：" + dataStr.toUtf8());
        }
    }

    else if(ui->checkBox_6->isChecked())
    {
        if(ui->checkBox_9->isChecked())
        {
            ui->textBrowser_2->append("发送组播包：" + dataStr.toUtf8());
        }
        mSocket->writeDatagram(dataStr.toUtf8(),QHostAddress(ui->lineEdit_12->text()),ui->lineEdit_4->text().toInt());
    }
}

void MainWindow::udpRecvData()
{
    QByteArray array;
    QHostAddress address;
    quint16 port;
    array.resize(mSocket->bytesAvailable());//根据可读数据来设置空间大小
    mSocket->readDatagram(array.data(),array.size(),&address,&port); //读取数据

    QString receiveMsg; 
    if(ui->checkBox_8->isChecked())
    {
        receiveMsg = ShowHex(array);
    }
    else
    {
        receiveMsg = QString::fromLocal8Bit(array);
    }

    ui->textBrowser_2->append("接收：" + receiveMsg);

}


void MainWindow::udpRecvSaveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save Data"), ".",
                                                    tr("Text File (*.txt)"));
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Save Data"),
                                                       tr("Cannot write file %1 : \n%2")
                                                       .arg(file.fileName())
                                                       .arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    out << ui->textBrowser_2->toPlainText();
    statusBar()->showMessage(tr("Data saved"), 2000);
    return;
}

void MainWindow::udpRecvClear()
{
    ui->textBrowser_2->clear();
}

void MainWindow::udpSendClear()
{
    ui->lineEdit_13->clear();
}

void MainWindow::multicastInfoShow()
{
    QMessageBox message(QMessageBox::NoIcon, "提示", "224.0.0.0～224.0.0.255为预留的组播地址；"
                                                   "224.0.1.0～224.0.1.255是公用组播地址；"
                                                   "224.0.2.0～238.255.255.255为用户可用的组播地址；"
                                                   "239.0.0.0～239.255.255.255为本地管理组播地址。");
    message.exec();
}


/*************************TCP部分接口*********************************/
void MainWindow::programmerTcpInit()
{
    ui->groupBox_10->hide();
    ui->groupBox_11->hide();
    ui->groupBox_12->hide();
    ui->groupBox_13->hide();
    ui->radioButton->setEnabled(false);
    ui->radioButton_2->setEnabled(false);
    connect(ui->pushButton_11, &QPushButton::clicked, this, &MainWindow::tcpServerInit);
    connect(ui->pushButton_12, &QPushButton::clicked, this, &MainWindow::tcpClientInit);
    connect(ui->pushButton_13, &QPushButton::clicked, this, &MainWindow::serverServiceInit);
    connect(ui->pushButton_14, &QPushButton::clicked, this, &MainWindow::clientServiceInit);
    connect(ui->pushButton_21, &QPushButton::clicked, this, &MainWindow::tcpSendData);
    connect(ui->pushButton_16, &QPushButton::clicked, this, &MainWindow::tcpRecvSaveAs);
    connect(ui->pushButton_15, &QPushButton::clicked, this, &MainWindow::tcpRecvClear);
    connect(ui->pushButton_17, &QPushButton::clicked, this, &MainWindow::tcpSendClear);
}

void MainWindow::tcpSendData()
{
    QString sendMessage = ui->lineEdit_15->text();
    if(ui->checkBox_13->isChecked())
    {
        QByteArray temp = QString2Hex(sendMessage);
        if((ui->radioButton->isChecked()) || (!ui->radioButton_2->isChecked()))
        {
            mTcpServerSocket->write(temp);
        }
        else if((!ui->radioButton->isChecked()) || (ui->radioButton_2->isChecked()))
        {
            mTcpClientSocket->write(temp);
        }

        if(ui->checkBox_12->isChecked())
        {
            ui->textBrowser_3->append("发送:" + ShowHex(temp));
        }
    }
    else
    {
        if((ui->radioButton->isChecked()) || (!ui->radioButton_2->isChecked()))
        {
            mTcpServerSocket->write(sendMessage.toLocal8Bit());
        }
        else if((!ui->radioButton->isChecked()) || (ui->radioButton_2->isChecked()))
        {
            mTcpClientSocket->write(sendMessage.toLocal8Bit());
        }

        if(ui->checkBox_12->isChecked())
        {
            ui->textBrowser_3->append("发送:" + sendMessage);
        }
    }
}

void MainWindow::tcpRecvSaveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save Data"), ".",
                                                    tr("Text File (*.txt)"));
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Save Data"),
                                                       tr("Cannot write file %1 : \n%2")
                                                       .arg(file.fileName())
                                                       .arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    out << ui->textBrowser_3->toPlainText();
    statusBar()->showMessage(tr("Data saved"), 2000);
    return;
}

void MainWindow::tcpRecvClear()
{
    ui->textBrowser_3->clear();
}

void MainWindow::tcpSendClear()
{
    ui->lineEdit_15->clear();
}

/*************SERVER部分接口***************/
void MainWindow::tcpServerInit()
{
    ui->groupBox_10->show();
    ui->groupBox_11->hide();
    ui->lineEdit_8->setEnabled(false);
    QString localIpStr = getTcpLocalIp();
    ui->lineEdit_8->setText(localIpStr);
}


void MainWindow::serverServiceInit()
{
    QString mString = ui->pushButton_13->text();
    if(mString == "关闭服务")
    {
        ui->pushButton_12->setEnabled(true);
        ui->pushButton_13->setText("开启服务");
        ui->lineEdit_8->clear();
        ui->lineEdit_9->clear();
        ui->label_12->setText("本地信息");
        ui->label_13->setText("连接信息");
        ui->groupBox_10->hide();
        ui->groupBox_12->hide();
        ui->groupBox_13->hide();
        ui->textBrowser_3->clear();
        ui->lineEdit_15->clear();
        disconnect(mTcpServer,SIGNAL(newConnection()), this,SLOT(newConnect()));
        mTcpServer->close();
        if(clientFlag == true)
        {
            mTcpServerSocket->close();
            clientFlag = false;
            ui->radioButton->setChecked(false);
        } 
        return; 
    }
    QString dataStr = ui->lineEdit_9->text();
    if(dataStr == 0)
    {
        QMessageBox::critical(this, tr("Critical Error"), "请输入端口号");
        return; 
    }
    mTcpServer->listen(QHostAddress::Any,ui->lineEdit_9->text().toInt());
    connect(mTcpServer,SIGNAL(newConnection()), this,SLOT(newConnect()));
    ui->label_12->setText("服务器:" + ui->lineEdit_8->text() + ":" + ui->lineEdit_9->text());
    ui->pushButton_13->setText("关闭服务");
    ui->pushButton_12->setEnabled(false);
}

void MainWindow::newConnect()
{
    mTcpServerSocket = mTcpServer->nextPendingConnection();
    clientFlag = true;
    //ui->label_13->setText("客户端:" + mTcpServerSocket->peerAddress().toString());
    ui->groupBox_12->show();
    ui->groupBox_13->show();
    connect(mTcpServerSocket,SIGNAL(readyRead()),this,SLOT(readMessage())); //服务器接收客户端的消息   
    connect(mTcpServerSocket,SIGNAL(disconnected()),this,SLOT(deleteSocket()));
    ui->radioButton->setChecked(true);
}

void MainWindow::readMessage()
{
    QByteArray array;
    array = mTcpServerSocket->readAll();

    QString receiveMsg; 
    if(ui->checkBox_11->isChecked())
    {
        receiveMsg = ShowHex(array);
    }
    else
    {
        receiveMsg = QString::fromLocal8Bit(array);
    }
    ui->textBrowser_3->append("接收：" + receiveMsg);
}


void MainWindow::deleteSocket()
{
    QMessageBox::critical(this, tr("Critical Error"), "连接断开");
    ui->label_13->setText("连接信息");
    ui->groupBox_12->hide();
    ui->groupBox_13->hide();
    clientFlag = false;
    ui->textBrowser_3->clear();
    ui->lineEdit_15->clear();
    ui->radioButton->setChecked(false);
}


/*************CLIENT部分接口***************/
void MainWindow::tcpClientInit()
{
    ui->groupBox_10->hide();
    ui->groupBox_11->show();
    ui->lineEdit_8->setEnabled(false);
    QString localIpStr = getTcpLocalIp();
    ui->lineEdit_10->setText(localIpStr);
}

void MainWindow::clientServiceInit()
{
    QString mString = ui->pushButton_14->text();
    if(mString == "关闭服务")
    {
        ui->pushButton_11->setEnabled(true);
        ui->pushButton_14->setText("开启服务");
        ui->lineEdit_11->clear();
        ui->lineEdit_14->clear();
        ui->groupBox_11->hide();
        ui->groupBox_12->hide();
        ui->groupBox_13->hide();
        ui->textBrowser_3->clear();
        ui->lineEdit_15->clear();
        if(clientConnectFlag == true)
        {
                mTcpClientSocket->close();
                clientConnectFlag = false;
        }
        return; 
    }
    QString IpStr = ui->lineEdit_14->text();
    if(IpStr == 0)
    {
        QMessageBox::critical(this, tr("Critical Error"), "请输入IP");
        return; 
    }

    QString portStr = ui->lineEdit_11->text();
    if(portStr == 0)
    {
        QMessageBox::critical(this, tr("Critical Error"), "请输入端口号");
        return; 
    }
    mTcpClientSocket->connectToHost(IpStr,portStr.toInt());
    connect(mTcpClientSocket,SIGNAL(connected()),this,SLOT(clientConnectSuccess()));
    connect(mTcpClientSocket,SIGNAL(disconnected()),this,SLOT(clientDisconnect()));
}

void MainWindow::clientReadMessage()
{
    QByteArray array;
    array = mTcpClientSocket->readAll();

    QString receiveMsg; 
    if(ui->checkBox_11->isChecked())
    {
        receiveMsg = ShowHex(array);
    }
    else
    {
        receiveMsg = QString::fromLocal8Bit(array);
    }
    ui->textBrowser_3->append("接收：" + receiveMsg);
    
}

void MainWindow::clientConnectSuccess()
{
    ui->groupBox_12->show();
    ui->groupBox_13->show();
    ui->pushButton_14->setText("关闭服务");
    ui->pushButton_11->setEnabled(false);
    connect(mTcpClientSocket,SIGNAL(readyRead()),this,SLOT(clientReadMessage()));
    clientConnectFlag = true;
    ui->radioButton_2->setChecked(true);
}

void MainWindow::clientDisconnect()
{
    QMessageBox::critical(this, tr("Critical Error"), "连接断开");
    ui->groupBox_12->hide();
    ui->groupBox_13->hide();
    ui->textBrowser_3->clear();
    ui->lineEdit_15->clear();
    clientConnectFlag = false;
    ui->radioButton_2->setChecked(false);
}


/*************************共用部分接口*********************************/
QString MainWindow::getTcpLocalIp()
{
    QString ipAddr;
    QList<QHostAddress> AddressList = QNetworkInterface::allAddresses();
    foreach(QHostAddress address, AddressList){
        if(address.protocol() == QAbstractSocket::IPv4Protocol &&
                address != QHostAddress::Null &&
                address != QHostAddress::LocalHost){
            if (address.toString().contains("127.0.")){
                continue;
            }
            ipAddr = address.toString();
            break;
        }
    }
    return ipAddr;
}


long MainWindow::atol_(const char* nptr)
{
    long total = 0;
    char sign = '+';
    while( isspace( *nptr ) ){ ++nptr; }                  // 跳过空格
    if( *nptr == '-' || *nptr == '+' ){ sign = *nptr++; } // 检查是否指定符号
    while( isdigit( *nptr ) ){
        total = 10 * total + ( (*nptr++) - '0' );
    }
    return (sign == '-') ? -total : total;
}


unsigned long MainWindow::inet_addr(char *cp )
{
    char ipBytes[4]={0};
    int i;
    for( i=0; i<4; i++, cp++ ){
        ipBytes[i] = (char)atol_( cp );
        if( !(cp = strchr( cp, '.' )) ){ break; }
    }
    return *(ULONG*)ipBytes;
}


QString MainWindow::ShowHex(QByteArray str)
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

char MainWindow::ConvertHexChar(char ch)
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
QByteArray MainWindow::QString2Hex(QString str)
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
bool MainWindow::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save Data"), ".",
                                                    tr("Text File (*.txt)"));
    if (fileName.isEmpty()) {
        return false;
    }
    return saveFile(fileName);
}

bool MainWindow::saveFile(const QString &fileName)
{
    if (!writeFile(fileName)) {
        statusBar()->showMessage(tr("Saving canceled"), 2000);
        return false;
    }
    statusBar()->showMessage(tr("Data saved"), 2000);
    return true;
}

bool MainWindow::writeFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Save Data"),
                                                       tr("Cannot write file %1 : \n%2")
                                                       .arg(file.fileName())
                                                       .arg(file.errorString()));
        return false;
    }
    QTextStream out(&file);
    out << ui->textBrowser->toPlainText();
    return true;
}

void MainWindow::showStatusMessage(const QString &message)
{
    ui->label_6->setText(message);
}


void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), programmerserial->serial->errorString());
    }
}
