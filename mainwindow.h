#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore/QtGlobal>
#include <QMainWindow>
#include <QDateTime>
#include <QTimer>
#include <QFileDialog>

#include <QtSerialPort/QSerialPort>

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

private slots:
	void handleError(QSerialPort::SerialPortError error);
	void dataSendBaseOnTimer();
	bool saveAs();


private:
    Ui::MainWindow *ui;

    programmerSerial *programmerserial;
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
