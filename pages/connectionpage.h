#pragma once

#include <QWidget>
#include <QObject>
#include <qcustomplot/qcustomplot.h>
#include <models/motionsample.h>
#include <models/portconfig.h>
#include <models/loglevel.h>

class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QComboBox;

class ConnectionPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectionPage(QWidget *parent = nullptr);

    void drawSeparator();
    void updateStream(const MotionPacket &packet);
    void setPorts(const QStringList &ports);
    PortConfig connectTo();

private:
    void createWidgets();
    void createLayouts();
    void connectSignals();
    void updateTime(const uint32_t time);

    void clearAccelGyroGraphs();
    void clearAllGraphs();
    void resetCountSnapshots() { snaphots = 0; }
    void updateGyroGraph(const MotionSample &sample);
    void updateAccelGraph(const MotionSample &sample);
    void updateliveDataPlotGraph(const MotionSample &sample);

    void addSeparator(QCustomPlot *plot, double x);
    void clearSeparators();
    void resetTime();

    // Layout
    QHBoxLayout *mainLayout;

    QVBoxLayout *settingsLayoutLeft;
    QVBoxLayout *settingsLayoutRight;
    QVBoxLayout *settingsLayoutPort;

    QHBoxLayout *portSettingsLayout;
    QHBoxLayout *opCloseLayout;

    // Widgets
    QComboBox *portList;
    QComboBox *baudRate;

    QPushButton *btnConnectPort;
    QPushButton *btnClosePort;
    QPushButton *updatePortList;

    QCustomPlot *gyroGraph;
    QCustomPlot *accelGraph;
    QCustomPlot *liveDataPlot;

    double tempTime = 0;
    double deltaTime = 0;

    bool lastRecordingState = false;

    int snaphots = 0;
    double timeStartSnaphots = 0;

     QList<QCPItemStraightLine*> m_separatorLines;

signals:
    void updatePortsClicked();
    void connectPortClicked();
    void closePortClicked();

    void logMessage(LogLevel level, const QString &text);
};
