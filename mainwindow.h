#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore/QtGlobal>
#include <QMainWindow>
#include <QDateTime>
#include <QTimer>
#include <QFileDialog>
#include <QUdpSocket>

#include <QtSerialPort/QSerialPort>
#include<QtNetwork/QTcpSocket>
#include<QtNetwork/QTcpServer>

namespace Ui {
class MainWindow;
}

class programmerSerial;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void programmerSerialInit();
    bool saveFile(const QString &fileName);
    bool writeFile(const QString &fileName);
    QString ShowHex(QByteArray str);
    char ConvertHexChar(char ch);
    QByteArray QString2Hex(QString str);
    unsigned long inet_addr(char *cp);
    long atol_(const char* nptr);

    void programmerUdpInit();
    void programmerTcpInit();

    QString getTcpLocalIp();

private slots:
	void handleError(QSerialPort::SerialPortError error);
	void dataSendBaseOnTimer();
	bool saveAs();

	void getLocalIp();
	void addMulticastInfo();
	void applyMulticastInfo();
	void udpInitDatagram();
	void udpSendData();
	void udpRecvData();
	void udpRecvSaveAs();
	void udpRecvClear();
	void udpSendClear();
	void multicastInfoShow();

	void tcpServerInit();
	void tcpClientInit();
	void serverServiceInit();
	void clientServiceInit();

	void newConnect();
	void readMessage();
	void tcpSendData();
	void deleteSocket();

	void clientReadMessage();
	void clientConnectSuccess();
	void clientDisconnect();

	void tcpRecvSaveAs();
	void tcpRecvClear();
	void tcpSendClear();

private:
    Ui::MainWindow *ui;

    programmerSerial *programmerserial;
    QUdpSocket *mSocket;
    QTcpSocket *mTcpServerSocket;
    QTcpSocket *mTcpClientSocket;
    QTcpServer *mTcpServer;
    bool clientFlag = false;
    bool clientConnectFlag = false;
    void getSerialInfo();
    void OpenSerial();
    void showStatusMessage(const QString &message);
    void readData();
    void writeData();
    void windowClear();

    void operateMenuInit();
    void settingMenuInit();
    void serialDisconnect();
    void programExit();

    void settingHide();
    void settingOpen();



};

#endif // MAINWINDOW_H
