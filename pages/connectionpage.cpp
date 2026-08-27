#include "pages/connectionpage.h"

#include "qcustomplot/qcustomplot.h"

#include <QDebug>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSplitter>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QStackedWidget>
#include <QGridLayout>

ConnectionPage::ConnectionPage(QWidget *parent)
    : QWidget(parent)
{
    createWidgets();
    createLayouts();
    connectSignals();
}

void ConnectionPage::createWidgets() {

    mainLayout = new QHBoxLayout(this);

    leftLayout = new QVBoxLayout();
    rigthLayout = new QVBoxLayout();

    leftWidget = new QWidget();
    leftWidget->setMaximumWidth(500);
    rightWidget = new QWidget();

    mainSplitter = new QSplitter(Qt::Horizontal, this);

// =================== LEFT SIDE

    settingsLayoutLeft = new QVBoxLayout();
    settingsLayoutPort = new QVBoxLayout();

    portSettingsLayout = new QHBoxLayout();
    opCloseLayout = new QHBoxLayout();

    portList = new QComboBox();

    baudRate = new QComboBox();
    baudRate->addItem("115200");
    baudRate->addItem("960000");

    btnConnectPort = new QPushButton("Connect to port");
    btnClosePort = new QPushButton("Disconect from port");
    updatePortList = new QPushButton("Update ports");
    \
// =================== RIGHT SIDE

    settingTypeMethod = new QHBoxLayout();
    settingDBName = new QHBoxLayout();
    settingSelectMovement = new QHBoxLayout();

    underGraphLayout = new QHBoxLayout();

    settingsRightLeftArea = new QVBoxLayout();
    settingsRightRightArea = new QVBoxLayout();

    methodOfrecord = new QComboBox();
    methodOfrecord->addItem("None");
    methodOfrecord->addItem("Once method");
    methodOfrecord->addItem("List method");

    motionType = new QComboBox();
    motionType->addItem("DoubleTap");
    motionType->addItem("SwipeLeft");
    motionType->addItem("SwipeRight");
    motionType->addItem("SwipeUp");
    motionType->addItem("SwipeDown");
    motionType->addItem("CircleCW");
    motionType->addItem("CircleCCW");
    motionType->addItem("Shake");
    motionType->addItem("NormalHandMovement");
    motionType->addItem("Walking");
    motionType->addItem("Unlabeled");
    motionType->addItem("Unknown");

    gyroGraph = new QCustomPlot();
    accelGraph= new QCustomPlot();
    liveDataPlot = new QCustomPlot();

    gyroGraph->setMinimumSize(200, 225);
    accelGraph->setMinimumSize(200, 225);
    liveDataPlot->setMinimumSize(400, 350);

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

    recordCheckBox = new QCheckBox("Start saves record");
    targetDbCombo = new QComboBox();

// =================== RECORDED DATA

    recordedDataGroup = new QGroupBox("Recorded Data");
    recordedDataStack = new QStackedWidget();

    // Page 0: None mode
    noneModeLabel = new QLabel("Recording disabled");
    noneModeLabel->setAlignment(Qt::AlignCenter);
    noneModeLabel->setStyleSheet("color: gray;");

    // Page 1: Once + List mode (shared page)
    onceStatusLabel = new QLabel("Buffer: 0 samples");
    btnSaveOnce = new QPushButton("Save");
    btnDiscardOnce = new QPushButton("Discard");
    btnPrevGesture = new QPushButton("<");
    btnNextGesture = new QPushButton(">");
    listIndexLabel = new QLabel("0 / 0");
    btnPrevGesture->setFixedWidth(40);
    btnNextGesture->setFixedWidth(40);

    QWidget *sharedPage = new QWidget();
    QHBoxLayout *sharedLayout = new QHBoxLayout(sharedPage);
    sharedLayout->setContentsMargins(0, 0, 0, 0);
    sharedLayout->addWidget(onceStatusLabel);
    sharedLayout->addStretch();
    sharedLayout->addWidget(btnPrevGesture);
    sharedLayout->addWidget(listIndexLabel);
    sharedLayout->addWidget(btnNextGesture);
    sharedLayout->addWidget(btnDiscardOnce);
    sharedLayout->addWidget(btnSaveOnce);

    recordedDataStack->addWidget(noneModeLabel);
    recordedDataStack->addWidget(sharedPage);
    recordedDataStack->setCurrentIndex(0);

    QVBoxLayout *recordedDataLayout = new QVBoxLayout(recordedDataGroup);
    recordedDataLayout->setContentsMargins(6, 6, 6, 6);
    recordedDataLayout->addWidget(recordedDataStack);

// =================== INFO BLOCK

    infoGroup = new QGroupBox("Sample Info");

    QLabel *lengthCaption = new QLabel("Length:");
    QLabel *freqCaption   = new QLabel("Freq:");
    QLabel *samplesCaption = new QLabel("Samples:");

    infoLengthLabel  = new QLabel("-- s");
    infoFreqLabel    = new QLabel("-- Hz");
    infoSamplesLabel = new QLabel("--");

    QGridLayout *infoGridLayout = new QGridLayout(infoGroup);
    infoGridLayout->setContentsMargins(6, 6, 6, 6);
    infoGridLayout->addWidget(lengthCaption,  0, 0);
    infoGridLayout->addWidget(infoLengthLabel, 0, 1);
    infoGridLayout->addWidget(freqCaption,    1, 0);
    infoGridLayout->addWidget(infoFreqLabel,  1, 1);
    infoGridLayout->addWidget(samplesCaption, 2, 0);
    infoGridLayout->addWidget(infoSamplesLabel, 2, 1);
    infoGridLayout->setColumnStretch(2, 1);

    btnPrevGesture->setEnabled(false);
    btnNextGesture->setEnabled(false);
    btnSaveOnce->setEnabled(false);
    btnDiscardOnce->setEnabled(false);

}

void ConnectionPage::createLayouts() {

// =================== LEFT SIDE

    portSettingsLayout->addWidget(new QLabel("Switch port:"));
    portSettingsLayout->addWidget(portList);
    portSettingsLayout->addWidget(new QLabel("  Switch baud rate:"));
    portSettingsLayout->addWidget(baudRate);
    portSettingsLayout->addStretch();
    portSettingsLayout->addWidget(updatePortList);

    opCloseLayout->addWidget(btnConnectPort);
    opCloseLayout->addWidget(btnClosePort);

    settingsLayoutPort->addLayout(portSettingsLayout);
    settingsLayoutPort->addLayout(opCloseLayout);

    leftLayout->addLayout(settingsLayoutPort);

    leftLayout->addWidget(liveDataPlot);
    leftLayout->addStretch(0);

// =================== RIGHT SIDE

    settingTypeMethod->addWidget(new QLabel("Choose method record: "));
    settingTypeMethod->addWidget(methodOfrecord);
    settingTypeMethod->addStretch();

    settingDBName->addWidget(new QLabel("Choose database for seves records: "));
    settingDBName->addWidget(targetDbCombo);
    settingDBName->addStretch();

    settingSelectMovement->addWidget(new QLabel("Select movement"));
    settingSelectMovement->addWidget(motionType);

    rigthLayout->addWidget(gyroGraph);
    rigthLayout->addWidget(accelGraph);

    settingsRightLeftArea->addLayout(settingSelectMovement);
    settingsRightLeftArea->addLayout(settingTypeMethod);
    settingsRightLeftArea->addLayout(settingDBName);
    settingsRightLeftArea->addWidget(recordCheckBox);

    settingsRightRightArea->addWidget(recordedDataGroup);

    underGraphLayout->addLayout(settingsRightLeftArea);
    underGraphLayout->addLayout(settingsRightRightArea);


    rigthLayout->addLayout(underGraphLayout);
    rigthLayout->addWidget(infoGroup);
    rigthLayout->addStretch();


// =================== SETUP PAGE

    leftWidget->setLayout(leftLayout);
    rightWidget->setLayout(rigthLayout);

    mainSplitter->addWidget(leftWidget);
    mainSplitter->addWidget(rightWidget);

    mainLayout->addWidget(mainSplitter);
    setLayout(mainLayout);

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

    connect(recordCheckBox,
            &QCheckBox::toggled,
            this,
            &ConnectionPage::recordingToggled);

    connect(targetDbCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int) { emit targetDatabaseChanged(targetDbCombo->currentText()); });

    connect(methodOfrecord,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int index) {
                recordedDataStack->setCurrentIndex(index == 0 ? 0 : 1);

                bool isOnce = (index == 1);
                btnPrevGesture->setEnabled(!isOnce);
                btnNextGesture->setEnabled(!isOnce);
                listIndexLabel->setVisible(!isOnce);
            });

    connect(btnSaveOnce,
            &QPushButton::clicked,
            this,
            &ConnectionPage::saveOnceRequested);

    connect(btnDiscardOnce,
            &QPushButton::clicked,
            this,
            &ConnectionPage::discardOnceRequested);

    // List mode navigation
    connect(btnPrevGesture,
            &QPushButton::clicked,
            this,
            &ConnectionPage::prevGestureRequested);

    connect(btnNextGesture,
            &QPushButton::clicked,
            this,
            &ConnectionPage::nextGestureRequested);
}

void ConnectionPage::drawSeparator() {
    addSeparator(liveDataPlot, tempTime);
    addSeparator(accelGraph, tempTime);
    addSeparator(gyroGraph, tempTime);
}

void ConnectionPage::addSeparator(QCustomPlot *plot, double x) {
    QCPItemStraightLine *line = new QCPItemStraightLine(plot);

    line->point1->setCoords(x, 0);
    line->point2->setCoords(x, 1);

    line->setPen(QPen(Qt::blue, 2, Qt::DashLine));

    m_separatorLines.append(line);

    plot->replot();
}

void ConnectionPage::clearSeparators() {
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
        emit logMessage(LogLevel::Info, QString("%1 packages arrived. Time motion: %2 sec. Freq(Hz): %3").arg(snaphots).arg(tempTime - timeStartSnaphots).arg(( snaphots / (tempTime - timeStartSnaphots))));
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

void ConnectionPage::setAvailableDatabases(const QStringList &databases)
{
    targetDbCombo->clear();
    targetDbCombo->addItems(databases);
}

PortConfig ConnectionPage::connectTo()
{
    PortConfig config;
    config.name = portList->currentText();
    config.baud = baudRate->currentText().toInt();
    return config;
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
