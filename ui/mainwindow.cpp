#include "ui/mainwindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>

#include <QDebug>

#include "widgets/leftmenuwidget.h"
#include "widgets/consolewidget.h"
#include "widgets/statusbarwidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(1400,900);

    createWidgets();
    createLayouts();
    connectSignals();
}

MainWindow::~MainWindow() {
    delete motionRecorder;
    serialPort->~SerialPort();
    dataBaseManager->~DatabaseManager();
}

void MainWindow::createWidgets() {
    serialPort = new SerialPort;
    dataBaseManager = new DatabaseManager;
    dataBaseManager->initialize();

    centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    leftMenu = new LeftMenuWidget;

    stack = new QStackedWidget;

    console = new ConsoleWidget;

    status = new StatusBarWidget;

    connectionPage = new ConnectionPage;

    motionRecorder = new MotionRecorder(dataBaseManager);

    datasetPage = new DataBasePage(dataBaseManager);

    livePage = new QWidget;

    sessionPage = new QWidget;

    stack->addWidget(connectionPage);
    stack->addWidget(livePage);
    stack->addWidget(sessionPage);
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

void MainWindow::connectSignals() {
    connect(leftMenu,
            &LeftMenuWidget::connectionClicked,
            this,
            [this]()
    {
        stack->setCurrentWidget(connectionPage);
    });

    connect(leftMenu,
            &LeftMenuWidget::liveClicked,
            this,
            [this]()
    {
        stack->setCurrentWidget(livePage);
    });

    connect(leftMenu,
            &LeftMenuWidget::sessionClicked,
            this,
            [this]()
    {
        stack->setCurrentWidget(sessionPage);
    });

    connect(leftMenu,
            &LeftMenuWidget::datasetClicked,
            this,
            [this]()
    {
        stack->setCurrentWidget(datasetPage);
    });

    connect(serialPort,
            &SerialPort::motionPacketReceived,
            connectionPage,
            &ConnectionPage::updateStream);

    connect(serialPort,
            &SerialPort::segmentEndReceived,
            connectionPage,
            &ConnectionPage::drawSeparator);

    connect(serialPort,
            &SerialPort::motionPacketReceived,
            motionRecorder,
            &MotionRecorder::onSample);

    connect(serialPort,
            &SerialPort::segmentEndReceived,
            motionRecorder,
            &MotionRecorder::onSegmentEnd);

    connect(connectionPage,
            &ConnectionPage::targetDatabaseChanged,
            motionRecorder,
            &MotionRecorder::setTargetDatabase);

    connect(connectionPage,
            &ConnectionPage::targetMethodChanged,
            motionRecorder,
            &MotionRecorder::setMethodSaves);

    connect(connectionPage,
            &ConnectionPage::targetMotionTypeChanged,
            motionRecorder,
            &MotionRecorder::setMotionType);

    connect(connectionPage,
            &ConnectionPage::saveOnceRequested,
            motionRecorder,
            &MotionRecorder::requestSaveGesture);

    connect(connectionPage,
            &ConnectionPage::discardOnceRequested,
            motionRecorder,
            &MotionRecorder::clearBuffer);

    connect(connectionPage,
            &ConnectionPage::trimRequested,
            motionRecorder,
            &MotionRecorder::trimBuffer);


    auto refreshDatabases = [this]() {
        connectionPage->setAvailableDatabases(dataBaseManager->getRegisteredDatabases());
    };

    connect(dataBaseManager,
            &DatabaseManager::databaseRegistered,
            this,
            refreshDatabases);
    refreshDatabases();

    connect(connectionPage,
            &ConnectionPage::updatePortsClicked,
            this,
            [this]()
    {
        connectionPage->setPorts(serialPort->updatePortList());
    });

    connect(connectionPage,
            &ConnectionPage::connectPortClicked,
            this,
            [this]()
    {
        serialPort->openPort(connectionPage->connectTo());
    });

    connect(connectionPage,
            &ConnectionPage::closePortClicked,
            this,
            [this]()
    {
        serialPort->closePort();
    });

    connect(serialPort,
            &SerialPort::connectionChanged,
            this,
            [this](ConnectionState state)
    {
        QString text;
        switch (state)
        {
        case ConnectionState::Connecting:   text = "Connecting...";  break;
        case ConnectionState::Connected:    text = "Connected";      break;
        case ConnectionState::Disconnected: text = "Disconnected";   break;
        }
        status->setStatus(text);
    });

    connect(serialPort,
            &SerialPort::logMessage,
            console,
            &ConsoleWidget::logMessage);

    connect(connectionPage,
            &ConnectionPage::logMessage,
            console,
            &ConsoleWidget::logMessage);

    connect(motionRecorder,
            &MotionRecorder::logMessage,
            console,
            &ConsoleWidget::logMessage);
    connect(motionRecorder,
            &MotionRecorder::sampleIsReady,
            connectionPage,
            &ConnectionPage::incomingSegment);

    connect(dataBaseManager,
            &DatabaseManager::logMessage,
            console,
            &ConsoleWidget::logMessage);

}
