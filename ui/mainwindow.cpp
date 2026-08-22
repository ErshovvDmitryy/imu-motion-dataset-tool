#include "ui/mainwindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>

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

MainWindow::~MainWindow()
{
}

void MainWindow::createWidgets()
{
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

    datasetPage = new DataBasePage(dataBaseManager);

    livePage = new QWidget;

    sessionPage = new QWidget;

    stack->addWidget(connectionPage);
    stack->addWidget(livePage);
    stack->addWidget(sessionPage);
    stack->addWidget(datasetPage);
}

void MainWindow::createLayouts()
{
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

void MainWindow::connectSignals()
{
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

    connect(dataBaseManager,
            &DatabaseManager::logMessage,
            console,
            &ConsoleWidget::logMessage);

}
