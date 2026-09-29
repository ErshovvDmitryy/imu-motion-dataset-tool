#pragma once

#include <QMainWindow>

#include "core/databasemanager.h"
#include "core/serialport.h"
#include "core/motionrecorder.h"
#include "models/datapacket.h"
#include "models/schemastore.h"


class QStackedWidget;
class LeftMenuWidget;
class ConsoleWidget;
class StatusBarWidget;
class ExportCSV;
class ConnectionPage;
class DataBasePage;
class MessageEditPage;
class PlotManager;
class MessageDecoder;
class PlotConfigStore;

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
    void registerMetaTypes();
    // Раздать всем страницам текущий набор схем.
    void publishSchemas();

    QWidget *centralWidget;

    LeftMenuWidget *leftMenu;

    QStackedWidget *stack;

    ConsoleWidget *console;

    StatusBarWidget *status;

    // Ядро: хранилище схем -> декодер -> потребители.
    SchemaStore *schemaStore;
    MessageDecoder *messageDecoder;
    PlotManager *plotManager;
    PlotConfigStore *plotConfigStore;

    ConnectionPage *connectionPage;
    DataBasePage *datasetPage;
    MessageEditPage *messageEditPage;
    ExportCSV *exportCSV;
    SerialPort *serialPort;
    DatabaseManager *dataBaseManager;
    MotionRecorder *motionRecorder = nullptr;
};
