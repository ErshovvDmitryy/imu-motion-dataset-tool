#pragma once

#include <QMainWindow>

#include "core/databasemanager.h"
#include "core/serialport.h"
#include "core/motionrecorder.h"
#include "pages/connectionpage.h"
#include "pages/databasepage.h"

class QStackedWidget;

class LeftMenuWidget;
class ConsoleWidget;
class StatusBarWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void createWidgets();
    void createLayouts();
    void connectSignals();

    QWidget *centralWidget;

    LeftMenuWidget *leftMenu;

    QStackedWidget *stack;

    ConsoleWidget *console;

    StatusBarWidget *status;

    ConnectionPage *connectionPage;
    DataBasePage *datasetPage;
    SerialPort *serialPort;
    DatabaseManager *dataBaseManager;
    MotionRecorder *motionRecorder = nullptr;

    QWidget *livePage;
    QWidget *sessionPage;
};
