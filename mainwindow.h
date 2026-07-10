#pragma once

#include <QMainWindow>

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

    QWidget *connectionPage;
    QWidget *livePage;
    QWidget *sessionPage;
    QWidget *datasetPage;
};
