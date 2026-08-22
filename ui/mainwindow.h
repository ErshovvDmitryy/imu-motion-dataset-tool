#pragma once

#include <QMainWindow>
#include "pages/connectionpage.h"
#include "pages/databasepage.h"
#include "core/serialport.h"
#include "core/databasemanager.h"

class QStackedWidget;
class QWidget;

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

    QWidget *livePage;
    QWidget *sessionPage;
};
