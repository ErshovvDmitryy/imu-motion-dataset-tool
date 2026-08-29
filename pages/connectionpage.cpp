#include "pages/connectionpage.h"

#include "qcustomplot/qcustomplot.h"

#include <QDebug>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSplitter>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QStackedWidget>
#include <QGridLayout>
#include <QMessageBox>
#include <QMouseEvent>

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
    rightLayout = new QVBoxLayout();

    leftWidget = new QWidget();
    leftWidget->setMaximumWidth(500);
    rightWidget = new QWidget();

    mainSplitter = new QSplitter(Qt::Horizontal, this);

    rightUnderGraphLayout = new QGridLayout();

// =================== LEFT SIDE===================

    settingsLayoutLeft = new QVBoxLayout();
    settingsLayoutPort = new QVBoxLayout();

    portSettingsLayout = new QHBoxLayout();
    portButtonsLayout = new QHBoxLayout();
    \

// =================== RIGHT SIDE ===================

    setupComboBox();            //  Setup combo box on page
    setupGraphs();              //  Setup graph on page
    setupButtonsOnPage();       //  Setup buttons on page

    setupDataBaseBlockWidget(); //  Setup db block
    setupRecordedDataWidget();  //  Setup recorded data block on page
    setupInfoBlockWidget();     //  Setup info block on page
    setupTrimBlockWidget();     //  Setup trim block on page

}

void ConnectionPage::createLayouts() {

// =================== LEFT SIDE ===================

    setupPortSettingsLayout(); // Include layout: settingsLayoutPort

    leftLayout->addLayout(settingsLayoutPort);
    leftLayout->addWidget(liveDataPlot);
    leftLayout->addStretch(0);

// =================== RIGHT SIDE ===================

    rightLayout->addWidget(gyroGraph);
    rightLayout->addWidget(accelGraph);

    rightLayout->addLayout(rightUnderGraphLayout);
    rightLayout->addStretch();

    rightUnderGraphLayout->addWidget(databaseBlock, 0, 0);
    rightUnderGraphLayout->addWidget(trimBlock, 0, 1);
    rightUnderGraphLayout->addWidget(infoGroup, 1, 0);
    rightUnderGraphLayout->addWidget(recordedDataGroup, 1, 1);

// =================== SETUP PAGE

    leftWidget->setLayout(leftLayout);
    rightWidget->setLayout(rightLayout);

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

    connect(targetDbCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int) { emit targetDatabaseChanged(targetDbCombo->currentText()); });

    connect(recordingMethod,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int) { emit targetMethodChanged(recordingMethod->currentText()); });

    connect(motionType,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int index) {
                MotionType type = static_cast<MotionType>(
                    motionType->itemData(index).toInt()
                );
                emit targetMotionTypeChanged(type);
            });

    connect(recordingMethod,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int index) {
                recordedDataStack->setCurrentIndex(index == 0 ? 0 : 1);

                bool isOnce = (index == 1);
                btnPrevGesture->setEnabled(!isOnce);
                btnNextGesture->setEnabled(!isOnce);
                listIndexLabel->setVisible(!isOnce);

                WorkMode mode = (index == 0) ? WorkMode::WORK_NONE
                             : (index == 1) ? WorkMode::WORK_ONCE
                                            : WorkMode::WORK_LIST;
                setWorkMode(mode);

                btnSaveOnce->setEnabled(false);
                btnDiscardOnce->setEnabled(false);

                btnTrimStart->setEnabled(isOnce);
                resetTrim();
            });

    connect(btnSaveOnce,
            &QPushButton::clicked,
            this,
            &ConnectionPage::onSaveOnceClicked);

    connect(btnDiscardOnce,
            &QPushButton::clicked,
            this,
            [this]() {
                emit discardOnceRequested();
                btnSaveOnce->setEnabled(false);
                btnDiscardOnce->setEnabled(false);
                onceStatusLabel->setText("Buffer: 0 samples");
                resetTrim();
            });

    connect(btnPrevGesture,
            &QPushButton::clicked,
            this,
            &ConnectionPage::prevGestureRequested);

    connect(btnNextGesture,
            &QPushButton::clicked,
            this,
            &ConnectionPage::nextGestureRequested);

    connect(btnTrimStart,
            &QPushButton::clicked,
            this,
            &ConnectionPage::onTrimButtonClicked);

    connect(btnTrimAccept,
            &QPushButton::clicked,
            this,
            &ConnectionPage::onTrimAccept);

    connect(btnTrimDeny,
            &QPushButton::clicked,
            this,
            &ConnectionPage::onTrimDeny);

    connect(accelGraph, &QCustomPlot::mousePress,
            this, [this](QMouseEvent *e) { onGraphMousePress(e, accelGraph); });
    connect(accelGraph, &QCustomPlot::mouseMove,
            this, [this](QMouseEvent *e) { onGraphMouseMove(e, accelGraph); });
    connect(accelGraph, &QCustomPlot::mouseRelease,
            this, [this](QMouseEvent *e) { onGraphMouseRelease(e, accelGraph); });

    connect(gyroGraph, &QCustomPlot::mousePress,
            this, [this](QMouseEvent *e) { onGraphMousePress(e, gyroGraph); });
    connect(gyroGraph, &QCustomPlot::mouseMove,
            this, [this](QMouseEvent *e) { onGraphMouseMove(e, gyroGraph); });
    connect(gyroGraph, &QCustomPlot::mouseRelease,
            this, [this](QMouseEvent *e) { onGraphMouseRelease(e, gyroGraph); });
}

void ConnectionPage::drawSeparator() {
    addSeparator(liveDataPlot, tempTime);
    addSeparator(accelGraph, tempTime);
    addSeparator(gyroGraph, tempTime);
}

void ConnectionPage::drawSavedSegmentSeparators() {
    if (motionSaved.isEmpty()) return;

    double x0 = motionSaved.first().time / 1000000.0;
    double x1 = motionSaved.last().time  / 1000000.0;

    addSeparator(liveDataPlot, x0);
    addSeparator(accelGraph,   x0);
    addSeparator(gyroGraph,    x0);
    addSeparator(liveDataPlot, x1);
    addSeparator(accelGraph,   x1);
    addSeparator(gyroGraph,    x1);
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

void ConnectionPage::addTrimLine(double sec, bool isStart) {
    auto *lineA = new QCPItemStraightLine(accelGraph);
    lineA->point1->setCoords(sec, 0);
    lineA->point2->setCoords(sec, 1);
    lineA->setPen(QPen(isStart ? Qt::green : Qt::red, 2, Qt::SolidLine));

    auto *lineG = new QCPItemStraightLine(gyroGraph);
    lineG->point1->setCoords(sec, 0);
    lineG->point2->setCoords(sec, 1);
    lineG->setPen(QPen(isStart ? Qt::green : Qt::red, 2, Qt::SolidLine));

    if (isStart) {
        for (QCPItemStraightLine *l : m_trimLinesStart)
            if (l && l->parentPlot()) l->parentPlot()->removeItem(l);
        m_trimLinesStart.clear();
        m_trimLinesStart.append(lineA);
        m_trimLinesStart.append(lineG);
    } else {
        for (QCPItemStraightLine *l : m_trimLinesEnd)
            if (l && l->parentPlot()) l->parentPlot()->removeItem(l);
        m_trimLinesEnd.clear();
        m_trimLinesEnd.append(lineA);
        m_trimLinesEnd.append(lineG);
    }
    accelGraph->replot(QCustomPlot::rpQueuedReplot);
    gyroGraph->replot(QCustomPlot::rpQueuedReplot);
}

void ConnectionPage::updateTrimLine(double sec, bool isStart) {
    const QVector<QCPItemStraightLine *> &vec = isStart ? m_trimLinesStart : m_trimLinesEnd;
    for (QCPItemStraightLine *line : vec) {
        line->point1->setCoords(sec, 0);
        line->point2->setCoords(sec, 1);
    }
    accelGraph->replot(QCustomPlot::rpQueuedReplot);
    gyroGraph->replot(QCustomPlot::rpQueuedReplot);
}

void ConnectionPage::clearTrimSeparators() {

    for (QCPItemStraightLine *line : m_trimLinesStart)
        if (line && line->parentPlot()) line->parentPlot()->removeItem(line);

    for (QCPItemStraightLine *line : m_trimLinesEnd)
        if (line && line->parentPlot()) line->parentPlot()->removeItem(line);
    m_trimLinesStart.clear();
    m_trimLinesEnd.clear();

    accelGraph->replot(QCustomPlot::rpQueuedReplot);
    gyroGraph->replot(QCustomPlot::rpQueuedReplot);
}

void ConnectionPage::setTrimInteractionEnabled(bool on) {

    if (on) {
        accelGraph->setInteraction(QCP::iRangeDrag, false);
        gyroGraph->setInteraction(QCP::iRangeDrag, false);
    } else {
        accelGraph->setInteraction(QCP::iRangeDrag, m_rangeDragAccel);
        gyroGraph->setInteraction(QCP::iRangeDrag, m_rangeDragGyro);
    }
}

void ConnectionPage::resetTrim() {
    clearTrimSeparators();
    m_trimStartSec = m_trimEndSec = -1;
    m_trimState = TrimState::Off;
    m_trimDragging = false;
    btnTrimAccept->setEnabled(false);
    btnTrimDeny->setEnabled(false);
    setTrimInteractionEnabled(false);
    if (recordTimeTrim) recordTimeTrim->setText("0 / 0");
}

double ConnectionPage::clampToData(double sec) const {
    if (motionSaved.isEmpty()) return sec;
    double first = motionSaved.first().time / 1000000.0;
    double last  = motionSaved.last().time / 1000000.0;
    return qBound(first, sec, last);
}

void ConnectionPage::onTrimButtonClicked() {
    if (!btnTrimStart->isEnabled()) return;

    if (m_trimState == TrimState::Off) {

        m_rangeDragAccel = accelGraph->interactions().testFlag(QCP::iRangeDrag);
        m_rangeDragGyro  = gyroGraph->interactions().testFlag(QCP::iRangeDrag);
        clearTrimSeparators();
        m_trimStartSec = m_trimEndSec = -1;
        m_trimState = TrimState::AwaitStart;
        btnTrimAccept->setEnabled(false);
        btnTrimDeny->setEnabled(false);
        setTrimInteractionEnabled(true);
        if (recordTimeTrim) recordTimeTrim->setText("click start / -");
    } else {

        resetTrim();
    }
}

void ConnectionPage::onGraphMousePress(QMouseEvent *event, QCustomPlot *plot) {
    if (m_trimState == TrimState::Off) return;

    double sec = clampToData(plot->xAxis->pixelToCoord(event->pos().x()));

    if (m_trimState == TrimState::AwaitStart) {
        m_trimStartSec = sec;
        addTrimLine(sec, true);
        m_trimState = TrimState::AwaitEnd;
        if (recordTimeTrim)
            recordTimeTrim->setText(QString("%1 / -").arg(sec, 0, 'f', 3));
    }
    else if (m_trimState == TrimState::AwaitEnd) {
        m_trimEndSec = sec;
        addTrimLine(sec, false);
        m_trimState = TrimState::Adjust;
        btnTrimAccept->setEnabled(true);
        btnTrimDeny->setEnabled(true);
        if (recordTimeTrim)
            recordTimeTrim->setText(QString("%1 / %2")
                .arg(m_trimStartSec, 0, 'f', 3)
                .arg(m_trimEndSec, 0, 'f', 3));
    }
    else if (m_trimState == TrimState::Adjust) {
        const double px = event->pos().x();
        if (m_trimStartSec >= 0) {
            double linePx = plot->xAxis->coordToPixel(m_trimStartSec);
            if (qAbs(linePx - px) <= TrimDragThresholdPx) {
                m_trimDragging = true;
                m_trimDragIsStart = true;
                return;
            }
        }
        if (m_trimEndSec >= 0) {
            double linePx = plot->xAxis->coordToPixel(m_trimEndSec);
            if (qAbs(linePx - px) <= TrimDragThresholdPx) {
                m_trimDragging = true;
                m_trimDragIsStart = false;
            }
        }
    }
}

void ConnectionPage::onGraphMouseMove(QMouseEvent *event, QCustomPlot *plot) {
    if (!m_trimDragging) return;
    double sec = clampToData(plot->xAxis->pixelToCoord(event->pos().x()));
    if (m_trimDragIsStart) m_trimStartSec = sec; else m_trimEndSec = sec;
    updateTrimLine(sec, m_trimDragIsStart);
    if (recordTimeTrim)
        recordTimeTrim->setText(QString("%1 / %2")
            .arg(m_trimStartSec, 0, 'f', 3)
            .arg(m_trimEndSec, 0, 'f', 3));
}

void ConnectionPage::onGraphMouseRelease(QMouseEvent *event, QCustomPlot *plot) {
    Q_UNUSED(event);
    Q_UNUSED(plot);
    m_trimDragging = false;
}

void ConnectionPage::onTrimAccept() {
    if (m_trimStartSec < 0 || m_trimEndSec < 0) return;

    double lo = (m_trimStartSec < m_trimEndSec) ? m_trimStartSec : m_trimEndSec;
    double hi = (m_trimStartSec < m_trimEndSec) ? m_trimEndSec : m_trimStartSec;

    if (!motionSaved.isEmpty()) {
        QVector<MotionSample> trimmed;
        for (const MotionSample &s : motionSaved) {
            double t = s.time / 1000000.0;
            if (t >= lo && t <= hi)
                trimmed.append(s);
        }
        motionSaved = trimmed;
        updateSavedGraph(motionSaved);
        drawSavedSegmentSeparators();
        onceStatusLabel->setText(QString("Buffer: %1 samples").arg(motionSaved.size()));
        btnSaveOnce->setEnabled(!motionSaved.isEmpty());
        btnDiscardOnce->setEnabled(!motionSaved.isEmpty());
    }

    emit trimRequested(lo, hi);

    resetTrim();
}

void ConnectionPage::onTrimDeny() {
    resetTrim();
}

void ConnectionPage::updateStream(const MotionPacket &packet) {

    if (workMode == WorkMode::WORK_NONE || workMode == WorkMode::WORK_ONCE) {

        updateTime(packet.sample.time);

        if (packet.recording && !lastRecordingState) {
            clearAccelGyroGraphs();
            addSeparator(liveDataPlot, tempTime);
            addSeparator(accelGraph, tempTime);
            addSeparator(gyroGraph, tempTime);
            timeStartSnaphots = tempTime;
            resetCountSnapshots();

            resetTrim();
        }

        updateliveDataPlotGraph(packet.sample);

        if (packet.recording) {
            updateAccelGraph(packet.sample);
            updateGyroGraph(packet.sample);
            snaphots++;
        }

        if (lastRecordingState && !packet.recording) {
            addSeparator(liveDataPlot, tempTime);
            addSeparator(accelGraph, tempTime);
            addSeparator(gyroGraph, tempTime);

            float length = tempTime - timeStartSnaphots;
            float freq = snaphots / ( tempTime - timeStartSnaphots );

            updateInfoBox( length, freq, snaphots );
        }

        lastRecordingState = packet.recording;
    }
}

void ConnectionPage::clearAccelGyroGraphs() {
    for (int i = 0; i < 3; i++) {
        accelGraph->graph(i)->data()->clear();
        gyroGraph->graph(i)->data()->clear();
    }

    clearSeparators();

    accelGraph->replot(QCustomPlot::rpQueuedReplot);
    gyroGraph->replot(QCustomPlot::rpQueuedReplot);
}

void ConnectionPage::clearAllGraphs() {
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

void ConnectionPage::resetTime() {
    tempTime = 0;
    deltaTime = 0;
}

void ConnectionPage::updateInfoBox(const float &infoLength,
                                     const float &infoFreq,
                                     const int infoSamples) {

    emit logMessage(LogLevel::Info, QString("%1 packages arrived. Time motion: %2 sec. Freq(Hz): %3").arg(infoSamples).arg(infoLength).arg(infoFreq));
    infoLengthLabel->setNum(infoLength);
    infoFreqLabel->setNum(infoFreq);
    infoSamplesLabel->setNum(infoSamples);
}

void ConnectionPage::updateSavedGraph(QVector<MotionSample> &motionSaved){

    if (motionSaved.isEmpty()) {
        return;
    }

    clearAllGraphs();
    updateTime(motionSaved.at(0).time);
    timeStartSnaphots = tempTime;

    for ( const MotionSample &sample : motionSaved ) {
        updateTime(sample.time);
        updateGyroGraph(sample);
        updateAccelGraph(sample);
        updateliveDataPlotGraph(sample);
    }

    float length = tempTime - timeStartSnaphots;
    float freq = motionSaved.size() / ( tempTime - timeStartSnaphots );
    updateInfoBox(length, freq, motionSaved.size());
}

void ConnectionPage::incomingSegment(QVector<MotionSample> &receivedData) {
    if (workMode == WorkMode::WORK_ONCE) {
        if (receivedData.isEmpty()) {
            return;
        }
        motionSaved = receivedData;
        updateSavedGraph(receivedData);
        drawSavedSegmentSeparators();

        btnSaveOnce->setEnabled(true);
        btnDiscardOnce->setEnabled(true);
        onceStatusLabel->setText(QString("Buffer: %1 samples").arg(receivedData.size()));
    }
}

void ConnectionPage::onSaveOnceClicked() {
    if (motionSaved.isEmpty()) {
        return;
    }

    const MotionSample &first = motionSaved.first();
    const MotionSample &last  = motionSaved.last();
    double duration = (last.time - first.time) / 1000000.0;
    int sampleCount = motionSaved.size();

    QString movement = motionType->currentText();
    QString database = targetDbCombo->currentText();

    QString msg = QString("Confirm saving gesture:\n\n"
                          "Movement: %1\n"
                          "Database: %2\n"
                          "Time: %3 s\n"
                          "Samples: %4")
            .arg(movement)
            .arg(database)
            .arg(duration, 0, 'f', 3)
            .arg(sampleCount);

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Save gesture",
        msg,
        QMessageBox::Save | QMessageBox::Cancel);

    if (reply == QMessageBox::Save) {

        emit saveOnceRequested();

        motionSaved.clear();
        btnSaveOnce->setEnabled(false);
        btnDiscardOnce->setEnabled(false);
        onceStatusLabel->setText("Saved");
    }
}

void ConnectionPage::setPorts(const QStringList &ports) {
    portList->clear();

    for (const QString &port : ports)
        portList->addItem(port);
}

void ConnectionPage::setAvailableDatabases(const QStringList &databases) {
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

void ConnectionPage::setWorkMode(WorkMode &mode)
{
    workMode = mode;
}

WorkMode ConnectionPage::getWorkMode() const {
    return workMode;
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

    liveDataPlot->xAxis->setRange(deltaTime, 5, Qt::AlignRight);
    liveDataPlot->yAxis->rescale(true);

    liveDataPlot->replot(QCustomPlot::rpQueuedReplot);
}

void ConnectionPage::setupGraphs() {

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
}

void ConnectionPage::setupComboBox() {

// =================== LEFT SIDE PAGE ===================

    portList = new QComboBox();

    baudRate = new QComboBox();
    baudRate->addItem("115200");
    baudRate->addItem("960000");

// =================== RIGHT SIDE PAGE ===================

}

void ConnectionPage::setupButtonsOnPage() {

// =================== LEFT SIDE PAGE ===================

    btnConnectPort = new QPushButton("Connect to port");
    btnClosePort = new QPushButton("Disconnect from port");
    updatePortList = new QPushButton("Update ports");

// =================== RIGHT SIDE PAGE ===================

    // Buttons declared in func group box
}

void ConnectionPage::setupRecordedDataWidget() {
    recordedDataGroup = new QGroupBox("Recorded Data");
    recordedDataStack = new QStackedWidget();

    noneModeLabel = new QLabel("Recording disabled");
    noneModeLabel->setAlignment(Qt::AlignCenter);
    noneModeLabel->setStyleSheet("color: gray;");

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
}

void ConnectionPage::setupInfoBlockWidget() {
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

void ConnectionPage::setupTrimBlockWidget() {
    trimBlock = new QGroupBox("Trim block");

    btnTrimStart = new QPushButton("Use trim");
    btnTrimAccept = new QPushButton("Accept");
    btnTrimDeny = new QPushButton("Deny");

    btnTrimStart->setFixedWidth(80);
    btnTrimAccept->setFixedWidth(80);
    btnTrimDeny->setFixedWidth(100);

    btnTrimStart->setEnabled(false);
    btnTrimAccept->setEnabled(false);
    btnTrimDeny->setEnabled(false);

    recordTimeTrim = new QLabel("0 / 0");

    QWidget *trimPage = new QWidget();
    QHBoxLayout *trimLayout = new QHBoxLayout(trimPage);
    trimLayout->setContentsMargins(0, 0, 0, 0);
    trimLayout->addWidget(btnTrimStart);
    trimLayout->addStretch();
    trimLayout->addWidget(recordTimeTrim);
    trimLayout->addStretch();
    trimLayout->addWidget(btnTrimAccept);
    trimLayout->addWidget(btnTrimDeny);

    QVBoxLayout *rightDownLayout = new QVBoxLayout(trimBlock);
    rightDownLayout->setContentsMargins(6, 6, 6, 6);
    rightDownLayout->addWidget(trimPage);
}

void ConnectionPage::setupDataBaseBlockWidget() {

    recordingMethod = new QComboBox();
    recordingMethod->addItem("None");
    recordingMethod->addItem("Once method");
    recordingMethod->addItem("List method");

    motionType = new QComboBox();
    motionType->addItem("DoubleTap", static_cast<int>(MotionType::DoubleTap));
    motionType->addItem("SwipeLeft", static_cast<int>(MotionType::SwipeLeft));
    motionType->addItem("SwipeRight", static_cast<int>(MotionType::SwipeRight));
    motionType->addItem("SwipeUp", static_cast<int>(MotionType::SwipeUp));
    motionType->addItem("SwipeDown", static_cast<int>(MotionType::SwipeDown));
    motionType->addItem("CircleCW", static_cast<int>(MotionType::CircleCW));
    motionType->addItem("CircleCCW", static_cast<int>(MotionType::CircleCCW));
    motionType->addItem("Shake", static_cast<int>(MotionType::Shake));
    motionType->addItem("NormalHandMovement", static_cast<int>(MotionType::NormalHandMovement));
    motionType->addItem("Walking", static_cast<int>(MotionType::Walking));
    motionType->addItem("Unlabeled", static_cast<int>(MotionType::Unlabeled));
    motionType->addItem("Unknown", static_cast<int>(MotionType::Unknown));

    targetDbCombo = new QComboBox();

    databaseBlock = new QGroupBox("DB Block");

    QWidget *dbBlock = new QWidget();

    QHBoxLayout *methodOfRecordLayout = new QHBoxLayout();
    methodOfRecordLayout->setContentsMargins(0, 0, 0, 0);
    methodOfRecordLayout->addWidget(new QLabel("Choose method record: "));
    methodOfRecordLayout->addWidget(recordingMethod);
    methodOfRecordLayout->addStretch();

    QHBoxLayout *selectMovementLayout = new QHBoxLayout();
    selectMovementLayout->addWidget(new QLabel("Select movement"));
    selectMovementLayout->addWidget(motionType);
    selectMovementLayout->addStretch();

    QHBoxLayout *selectDataBaseLayout = new QHBoxLayout();
    selectDataBaseLayout->addWidget(new QLabel("Select database for save records"));
    selectDataBaseLayout->addWidget(targetDbCombo);
    selectDataBaseLayout->addStretch();

    QVBoxLayout *dbLayout = new QVBoxLayout(dbBlock);
    dbLayout->addLayout(methodOfRecordLayout);
    dbLayout->addLayout(selectMovementLayout);
    dbLayout->addLayout(selectDataBaseLayout);

    QHBoxLayout *topLeftLayout = new QHBoxLayout(databaseBlock);
    topLeftLayout->setContentsMargins(6, 6, 6, 6);
    topLeftLayout->addWidget(dbBlock);
}

void ConnectionPage::setupPortSettingsLayout() {
    portSettingsLayout->addWidget(updatePortList);
    portSettingsLayout->addStretch();
    portSettingsLayout->addWidget(new QLabel("Select port:"));
    portSettingsLayout->addWidget(portList);
    portSettingsLayout->addWidget(new QLabel("  Switch baud rate:"));
    portSettingsLayout->addWidget(baudRate);

    portButtonsLayout->addWidget(btnConnectPort);
    portButtonsLayout->addWidget(btnClosePort);

    settingsLayoutPort->addLayout(portSettingsLayout);
    settingsLayoutPort->addLayout(portButtonsLayout);
}
