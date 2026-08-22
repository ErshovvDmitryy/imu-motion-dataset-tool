#include "pages/connectionpage.h"

#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>

ConnectionPage::ConnectionPage(QWidget *parent)
    : QWidget(parent)
{
    createWidgets();
    createLayouts();
    connectSignals();
}

void ConnectionPage::drawSeparator()
{
    addSeparator(liveDataPlot, tempTime);
    addSeparator(accelGraph, tempTime);
    addSeparator(gyroGraph, tempTime);
}

void ConnectionPage::addSeparator(QCustomPlot *plot, double x)
{
    QCPItemStraightLine *line = new QCPItemStraightLine(plot);

    line->point1->setCoords(x, 0);
    line->point2->setCoords(x, 1);

    line->setPen(QPen(Qt::blue, 2, Qt::DashLine));

    m_separatorLines.append(line);

    plot->replot();
}

void ConnectionPage::clearSeparators()
{
    for (QCPItemStraightLine *line : m_separatorLines) {
        if (line->parentPlot()) {
            line->parentPlot()->removeItem(line);
        }
    }
    m_separatorLines.clear();
}

void ConnectionPage::updateStream(const MotionPacket &packet)
{
    updateTime(packet.sample.time);

    if (packet.recording && !lastRecordingState) {
        clearAccelGyroGraphs();
        addSeparator(accelGraph, tempTime);
        addSeparator(gyroGraph, tempTime);
        timeStartSnaphots = tempTime;
        resetCountSnapshots();
    }

    updateliveDataPlotGraph(packet.sample);

    if (packet.recording) {
        updateAccelGraph(packet.sample);
        updateGyroGraph(packet.sample);
        snaphots++;
    }

    if (lastRecordingState && !packet.recording) {
        addSeparator(accelGraph, tempTime);
        addSeparator(gyroGraph, tempTime);
        emit logMessage(LogLevel::Info, QString("%1 packages arrived. Time motion: %2 sec").arg(snaphots).arg(tempTime - timeStartSnaphots));
    }

    lastRecordingState = packet.recording;
}

void ConnectionPage::clearAccelGyroGraphs()
{
    for (int i = 0; i < 3; i++) {
        accelGraph->graph(i)->data()->clear();
        gyroGraph->graph(i)->data()->clear();
    }

    clearSeparators();

    accelGraph->replot(QCustomPlot::rpQueuedReplot);
    gyroGraph->replot(QCustomPlot::rpQueuedReplot);
}

void ConnectionPage::clearAllGraphs()
{
    for (int i = 0; i < 3; i++) {
        accelGraph->graph(i)->data()->clear();
        gyroGraph->graph(i)->data()->clear();
        liveDataPlot->graph(i)->data()->clear();
    }

    clearSeparators();
    resetTime();

    accelGraph->replot(QCustomPlot::rpQueuedReplot);
    gyroGraph->replot(QCustomPlot::rpQueuedReplot);
    liveDataPlot->replot(QCustomPlot::rpQueuedReplot);
}

void ConnectionPage::resetTime()
{
    tempTime = 0;
    deltaTime = 0;
}


void ConnectionPage::setPorts(const QStringList &ports)
{
    portList->clear();

    for (const QString &port : ports)
        portList->addItem(port);
}

PortConfig ConnectionPage::connectTo()
{
    PortConfig config;
    config.name = portList->currentText();
    config.baud = baudRate->currentText().toInt();
    return config;
}

void ConnectionPage::createWidgets() {
    portList = new QComboBox;

    baudRate = new QComboBox;
    baudRate->addItem("115200");
    baudRate->addItem("960000");

    btnConnectPort = new QPushButton("Подключиться");
    btnClosePort = new QPushButton("Отключиться");
    updatePortList = new QPushButton("Обновить");

    gyroGraph = new QCustomPlot();
    accelGraph= new QCustomPlot();
    liveDataPlot = new QCustomPlot();

    gyroGraph->setMinimumSize(200, 150);
    accelGraph->setMinimumSize(200, 150);
    liveDataPlot->setMinimumSize(200, 150);
}

void ConnectionPage::createLayouts() {
    mainLayout = new QHBoxLayout(this);

    for (int i = 0; i < 3; i++){
        gyroGraph->addGraph();
        accelGraph->addGraph();
        liveDataPlot->addGraph();
    }

    gyroGraph->graph(0)->setPen(QPen(Qt::red));
    accelGraph->graph(0)->setPen(QPen(Qt::red));

    gyroGraph->graph(1)->setPen(QPen(Qt::green));
    accelGraph->graph(1)->setPen(QPen(Qt::green));

    gyroGraph->graph(2)->setPen(QPen(Qt::blue));
    accelGraph->graph(2)->setPen(QPen(Qt::blue));

    liveDataPlot->graph(0)->setPen(QPen(Qt::magenta, 2));

    gyroGraph->xAxis->setLabel("Time (s)");
    gyroGraph->yAxis->setLabel("Gyro (°/с)");
    accelGraph->xAxis->setLabel("Time (s)");
    accelGraph->yAxis->setLabel("Accel (g)");
    liveDataPlot->xAxis->setLabel("Time (s)");
    liveDataPlot->yAxis->setLabel("Total Accel (g)");

    settingsLayoutLeft = new QVBoxLayout;
    settingsLayoutRight = new QVBoxLayout;
    settingsLayoutPort = new QVBoxLayout;

    portSettingsLayout = new QHBoxLayout;
    opCloseLayout = new QHBoxLayout;

    portSettingsLayout->addWidget(portList);
    portSettingsLayout->addWidget(updatePortList);
    portSettingsLayout->addWidget(baudRate);

    settingsLayoutRight->addWidget(gyroGraph);
    settingsLayoutRight->addWidget(accelGraph);
    settingsLayoutRight->addWidget(liveDataPlot);

    opCloseLayout->addWidget(btnConnectPort);
    opCloseLayout->addWidget(btnClosePort);

    settingsLayoutPort->addLayout(portSettingsLayout);
    settingsLayoutPort->addLayout(opCloseLayout);
    settingsLayoutPort->addStretch();

    settingsLayoutLeft->addLayout(settingsLayoutPort);

    settingsLayoutRight->addStretch();

    mainLayout->addLayout(settingsLayoutLeft);
    mainLayout->addLayout(settingsLayoutRight);

    mainLayout->setStretch(0, 1);
    mainLayout->setStretch(1, 2);
}

void ConnectionPage::connectSignals() {

    connect(updatePortList,
            &QPushButton::clicked,
            this,
            &ConnectionPage::updatePortsClicked);

    connect(btnConnectPort,
            &QPushButton::clicked,
            this,
            &ConnectionPage::connectPortClicked);

    connect(btnClosePort,
            &QPushButton::clicked,
            this,
            &ConnectionPage::closePortClicked);

    connect(btnClosePort,
            &QPushButton::clicked,
            this,
            &ConnectionPage::clearAllGraphs);
}

void ConnectionPage::updateTime(const uint32_t t)
{
    tempTime = t / 1000000.0;
    deltaTime = tempTime;
}

void ConnectionPage::updateAccelGraph(const MotionSample &sample)
{
    accelGraph->graph(0)->addData(deltaTime, sample.ax);
    accelGraph->graph(1)->addData(deltaTime, sample.ay);
    accelGraph->graph(2)->addData(deltaTime, sample.az);

    accelGraph->xAxis->setRange(deltaTime, 5, Qt::AlignCenter);
    accelGraph->yAxis->rescale(true);

    accelGraph->replot(QCustomPlot::rpQueuedReplot);
}

void ConnectionPage::updateGyroGraph(const MotionSample &sample)
{
    gyroGraph->graph(0)->addData(deltaTime, sample.gx);
    gyroGraph->graph(1)->addData(deltaTime, sample.gy);
    gyroGraph->graph(2)->addData(deltaTime, sample.gz);

    gyroGraph->xAxis->setRange(deltaTime, 5, Qt::AlignCenter);
    gyroGraph->yAxis->rescale(true);

    gyroGraph->replot(QCustomPlot::rpQueuedReplot);
}

void ConnectionPage::updateliveDataPlotGraph(const MotionSample &sample)
{
    float totalAccel = sqrt(
        sample.ax * sample.ax +
        sample.ay * sample.ay +
        sample.az * sample.az
    );

    liveDataPlot->graph(0)->addData(deltaTime, totalAccel);

    liveDataPlot->xAxis->setRange(deltaTime, 5, Qt::AlignCenter);
    liveDataPlot->yAxis->rescale(true);

    liveDataPlot->replot(QCustomPlot::rpQueuedReplot);
}
