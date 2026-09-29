#include "ui/mainwindow.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "core/protocol/messagedecoder.h"
#include "models/builtinschemas.h"
#include "modules/plot/plotconfig.h"
#include "modules/widgets/plotmanager.h"
#include "pages/connectionpage.h"
#include "pages/databasepage.h"
#include "pages/exportcsv.h"
#include "pages/messageeditpage.h"
#include "widgets/consolewidget.h"
#include "widgets/leftmenuwidget.h"
#include "widgets/statusbarwidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(1400,900);

    registerMetaTypes();
    createWidgets();
    createLayouts();
    connectSignals();

    // Схемы нужны до первого кадра: без них парсер не знает ни одного
    // typeId и всё, что приходит, считается мусором.
    schemaStore->ensureBuiltins();
    schemaStore->loadAll();
    publishSchemas();
}

MainWindow::~MainWindow() {
}

void MainWindow::registerMetaTypes() {
    // Кадры ходят через сигналы, поэтому тип должен быть известен Qt.
    qRegisterMetaType<DataFrame>("DataFrame");
    qRegisterMetaType<DataSchema>("DataSchema");
    qRegisterMetaType<PlottedField>("PlottedField");
    qRegisterMetaType<QVector<DataFrame> >("QVector<DataFrame>");
}

void MainWindow::createWidgets() {
    serialPort = new SerialPort;
    dataBaseManager = new DatabaseManager;
    dataBaseManager->initialize();

    schemaStore = new SchemaStore(this);
    messageDecoder = new MessageDecoder(this);
    plotManager = new PlotManager(this);
    plotConfigStore = new PlotConfigStore(this);
    plotConfigStore->load();

    centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    leftMenu = new LeftMenuWidget;
    stack = new QStackedWidget;
    console = new ConsoleWidget;
    status = new StatusBarWidget;

    // Страницы создаются после ядра: графики создаются PlotManager.
    connectionPage = new ConnectionPage(plotManager, plotConfigStore);
    datasetPage = new DataBasePage(dataBaseManager, plotManager, plotConfigStore);
    messageEditPage = new MessageEditPage(schemaStore);
    exportCSV = new ExportCSV(dataBaseManager, nullptr);

    motionRecorder = new MotionRecorder;
    motionRecorder->setDatabaseManager(dataBaseManager);

    stack->addWidget(connectionPage);
    stack->addWidget(messageEditPage);
    stack->addWidget(exportCSV);
    stack->addWidget(datasetPage);
}

void MainWindow::createLayouts() {
    auto *mainLayout = new QHBoxLayout(centralWidget);
    auto *rightLayout = new QVBoxLayout;

    rightLayout->addWidget(stack);
    rightLayout->addWidget(console);
    rightLayout->addWidget(status);

    mainLayout->addWidget(leftMenu);
    mainLayout->addLayout(rightLayout);

    mainLayout->setStretch(0,1);
    mainLayout->setStretch(1,6);
}

void MainWindow::publishSchemas() {
    const QList<DataSchema> schemas = schemaStore->schemas();
    messageDecoder->setSchemas(schemas);
    motionRecorder->setSchemas(schemas);
    connectionPage->setSchemas(schemas);
}

void MainWindow::connectSignals() {
    connect(leftMenu, &LeftMenuWidget::connectionClicked, this, [this]() {
        stack->setCurrentWidget(connectionPage);
    });
    connect(leftMenu, &LeftMenuWidget::messagesClicked, this, [this]() {
        stack->setCurrentWidget(messageEditPage);
    });
    connect(leftMenu, &LeftMenuWidget::exportClicked, this, [this]() {
        stack->setCurrentWidget(exportCSV);
    });
    connect(leftMenu, &LeftMenuWidget::datasetClicked, this, [this]() {
        stack->setCurrentWidget(datasetPage);
    });

    // ---- транспорт -> протокол

    connect(serialPort, &SerialPort::rawDataReceived, messageDecoder, &MessageDecoder::onRawData);

    // ---- протокол -> потребители

    connect(messageDecoder, &MessageDecoder::frameReceived, this, [this](const DataFrame &frame) {
        // Граница сегмента рисуется один раз, а не на каждый кадр.
        if (frame.hasCrcField() == false) {
            connectionPage->onFrame(frame);
        }
        motionRecorder->onFrame(frame);
    });

    connect(messageDecoder, &MessageDecoder::frameReceived, connectionPage,
            [this](const DataFrame &frame) {
                if (frame.hasCrcField()) {
                    bool ok = false;
                    const double seconds = frame.timeSeconds(&ok);
                    connectionPage->onSegmentBoundary(ok ? seconds : -1.0);
                }
            });

    connect(messageDecoder, &MessageDecoder::logMessage, console, &ConsoleWidget::logMessage);
    connect(schemaStore, &SchemaStore::logMessage, console, &ConsoleWidget::logMessage);

    // Правка схем в редакторе сразу меняет поведение парсера и графиков.
    connect(schemaStore, &SchemaStore::schemasReloaded, this, &MainWindow::publishSchemas);
    connect(schemaStore, &SchemaStore::schemaSaved, this, [this](const QString &) {
        publishSchemas();
    });
    connect(schemaStore, &SchemaStore::schemaRemoved, this, [this](const QString &) {
        publishSchemas();
    });

    // ---- страница подключения -> буфер записи

    connect(connectionPage, &ConnectionPage::targetDatabaseChanged, motionRecorder,
            &MotionRecorder::setTargetDatabase);
    connect(connectionPage, &ConnectionPage::targetMethodChanged, motionRecorder,
            &MotionRecorder::setMethodSaves);
    connect(connectionPage, &ConnectionPage::targetMotionTypeChanged, motionRecorder,
            &MotionRecorder::setMotionType);
    connect(connectionPage, &ConnectionPage::trackedSchemasChanged, motionRecorder,
            &MotionRecorder::setRecordedSchemas);
    connect(connectionPage, &ConnectionPage::prevGestureRequested, motionRecorder,
            &MotionRecorder::prevGesture);
    connect(connectionPage, &ConnectionPage::nextGestureRequested, motionRecorder,
            &MotionRecorder::nextGesture);
    connect(connectionPage, &ConnectionPage::saveOnceRequested, motionRecorder,
            &MotionRecorder::requestSaveGesture);
    connect(connectionPage, &ConnectionPage::discardOnceRequested, motionRecorder,
            &MotionRecorder::clearBuffer);
    connect(connectionPage, &ConnectionPage::trimRequested, motionRecorder,
            &MotionRecorder::trimBuffer);
    connect(connectionPage, &ConnectionPage::flagMarkingRequested, motionRecorder,
            &MotionRecorder::setFlags);

    connect(motionRecorder, &MotionRecorder::sampleIsReady, connectionPage,
            &ConnectionPage::showSavedFrames);
    connect(motionRecorder, &MotionRecorder::selectedSample, connectionPage,
            &ConnectionPage::showSavedFrames);
    connect(motionRecorder, &MotionRecorder::updateSizeList, connectionPage,
            &ConnectionPage::sizeListUpdate);
    connect(motionRecorder, &MotionRecorder::logMessage, console, &ConsoleWidget::logMessage);

    // ---- базы данных

    const auto refreshDatabases = [this]() {
        connectionPage->setAvailableDatabases(dataBaseManager->getRegisteredDatabases());
    };
    connect(dataBaseManager, &DatabaseManager::databaseRegistered, this, refreshDatabases);
    refreshDatabases();

    connect(dataBaseManager, &DatabaseManager::logMessage, console, &ConsoleWidget::logMessage);
    connect(exportCSV, &ExportCSV::logMessage, console, &ConsoleWidget::logMessage);
    connect(connectionPage, &ConnectionPage::logMessage, console, &ConsoleWidget::logMessage);
    connect(plotManager, &PlotManager::logMessage, console, &ConsoleWidget::logMessage);
    connect(plotConfigStore, &PlotConfigStore::logMessage, this,
            [this](const QString &text) { console->logMessage(LogLevel::Info, text); });

    // ---- порт

    connect(connectionPage, &ConnectionPage::updatePortsClicked, this, [this]() {
        connectionPage->setPorts(serialPort->updatePortList());
    });
    connect(connectionPage, &ConnectionPage::connectPortClicked, this, [this]() {
        serialPort->openPort(connectionPage->connectTo());
    });
    connect(connectionPage, &ConnectionPage::closePortClicked, this, [this]() {
        serialPort->closePort();
    });

    connect(serialPort, &SerialPort::connectionChanged, this, [this](ConnectionState state) {
        QString text;
        switch (state)
        {
        case ConnectionState::Connecting:   text = "Connecting...";  break;
        case ConnectionState::Connected:    text = "Connected";      break;
        case ConnectionState::Disconnected: text = "Disconnected";   break;
        }
        status->setStatus(text);
        if (state == ConnectionState::Disconnected) {
            messageDecoder->reset();
        }
    });

    connect(serialPort, &SerialPort::logMessage, console, &ConsoleWidget::logMessage);
}
