#include "MainWindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>

#include "LeftMenuWidget.h"
#include "ConsoleWidget.h"
#include "StatusBarWidget.h"

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
    centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    leftMenu = new LeftMenuWidget;

    stack = new QStackedWidget;

    console = new ConsoleWidget;

    status = new StatusBarWidget;

    connectionPage = new QWidget;
    livePage = new QWidget;
    sessionPage = new QWidget;
    datasetPage = new QWidget;

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

}
