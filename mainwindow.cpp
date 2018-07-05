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
    operateMenuInit();        //操作菜单初始化
    settingMenuInit();        //设置菜单初始化
    programmerSerialInit();   //串口部分初始化

}

MainWindow::~MainWindow()
{
    delete ui;
    delete programmerserial;
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


/*串口部分接口*/
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
    if(data.endsWith("\n"))
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
