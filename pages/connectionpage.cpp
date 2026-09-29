#include "pages/connectionpage.h"

#include "qcustomplot/qcustomplot.h"
#include "modules/widgets/plotmanager.h"
#include "modules/plot/plotconfig.h"
#include "models/builtinschemas.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

const char *kSlotLive = "connection/live";
const char *kSlotAccel = "connection/accel";
const char *kSlotGyro = "connection/gyro";

QString builtin(const char *name) {
    return QString::fromLatin1(name);
}

} // namespace

ConnectionPage::ConnectionPage(PlotManager *plotManager,
                               PlotConfigStore *plotConfigStore,
                               QWidget *parent)
    : QWidget(parent)
    , m_plotManager(plotManager)
    , m_plotConfigStore(plotConfigStore)
{
    createWidgets();
    createLayouts();
    connectSignals();
}

// ======================================================================
//  Создание виджетов
// ======================================================================

void ConnectionPage::createWidgets() {
    mainLayout = new QHBoxLayout(this);
    leftLayout = new QVBoxLayout();
    rightLayout = new QVBoxLayout();

    leftWidget = new QWidget();
    leftWidget->setMaximumWidth(500);
    rightWidget = new QWidget();
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    rightUnderGraphLayout = new QGridLayout();

    settingsLayoutLeft = new QVBoxLayout();
    settingsLayoutPort = new QVBoxLayout();
    portSettingsLayout = new QHBoxLayout();
    portButtonsLayout = new QHBoxLayout();

    setupComboBox();
    setupGraphs();
    setupButtonsOnPage();

    setupDataBaseBlockWidget();
    setupRecordedDataWidget();
    setupInfoBlockWidget();
    setupTrimBlockWidget();
    setupFlagMarkingWidget();
    setupTrackingWidget();
}

void ConnectionPage::setupGraphs() {
    m_schemas = m_plotManager->schemas();
    applyAllPlotConfigs();
}

PlotConfig ConnectionPage::configFromStore(const QString &slotId, const PlotConfig &fallback) const {
    if (m_plotConfigStore != nullptr && m_plotConfigStore->has(slotId)) {
        const PlotConfig stored = m_plotConfigStore->config(slotId);
        QString error;
        if (stored.isValid(&error)) {
            return stored;
        }
    }
    return fallback;
}

PlotConfig ConnectionPage::defaultLiveConfig() const {
    const QString imu = builtin(BuiltinSchemas::imuName);

    PlotConfig config;
    config.schemaName = imu;
    config.title = QStringLiteral("Live");
    config.xMode = XAxisMode::Timestamp;
    config.liveWindowSec = 5.0;
    config.relativeTime = true;
    config.showLegend = false;
    config.xLabel = QStringLiteral("Time (s)");
    config.yLabel = QStringLiteral("Accel (g)");
    // Модуль вектора в схеме не выразить, поэтому показываем оси
    // акселерометра. Свой набор полей меняется через контекстное меню.
    config.traces.append(PlotTrace{ QString::fromLatin1(BuiltinSchemas::ImuField::ax), QColor() });
    config.traces.append(PlotTrace{ QString::fromLatin1(BuiltinSchemas::ImuField::ay), QColor() });
    config.traces.append(PlotTrace{ QString::fromLatin1(BuiltinSchemas::ImuField::az), QColor() });
    config.applyDefaultColors();
    return config;
}

PlotConfig ConnectionPage::defaultAccelConfig() const {
    PlotConfig config;
    config.schemaName = builtin(BuiltinSchemas::imuName);
    config.title = QStringLiteral("Accelerometer");
    config.xMode = XAxisMode::Timestamp;
    config.liveWindowSec = 5.0;
    config.relativeTime = true;
    config.xLabel = QStringLiteral("Time (s)");
    config.yLabel = QStringLiteral("Accel (g)");
    for (const char *field : { BuiltinSchemas::ImuField::ax, BuiltinSchemas::ImuField::ay,
                               BuiltinSchemas::ImuField::az }) {
        config.traces.append(PlotTrace{ QString::fromLatin1(field), QColor() });
    }
    config.applyDefaultColors();
    return config;
}

PlotConfig ConnectionPage::defaultGyroConfig() const {
    PlotConfig config;
    config.schemaName = builtin(BuiltinSchemas::imuName);
    config.title = QStringLiteral("Gyroscope");
    config.xMode = XAxisMode::Timestamp;
    config.liveWindowSec = 5.0;
    config.relativeTime = true;
    config.xLabel = QStringLiteral("Time (s)");
    config.yLabel = QStringLiteral("Gyro (dps)");
    for (const char *field : { BuiltinSchemas::ImuField::gx, BuiltinSchemas::ImuField::gy,
                               BuiltinSchemas::ImuField::gz }) {
        config.traces.append(PlotTrace{ QString::fromLatin1(field), QColor() });
    }
    config.applyDefaultColors();
    return config;
}

QCustomPlot *ConnectionPage::plotForSlot(const QString &slotId) {
    const QString live = QLatin1String(kSlotLive);
    const QString accel = QLatin1String(kSlotAccel);
    const QString gyro = QLatin1String(kSlotGyro);

    if (slotId == live) {
        return liveDataPlot;
    }
    if (slotId == accel) {
        return accelGraph;
    }
    if (slotId == gyro) {
        return gyroGraph;
    }
    return nullptr;
}

void ConnectionPage::applyAllPlotConfigs() {
    if (m_plotManager == nullptr) {
        return;
    }

    const QString liveSlot = QLatin1String(kSlotLive);
    const QString accelSlot = QLatin1String(kSlotAccel);
    const QString gyroSlot = QLatin1String(kSlotGyro);

    if (liveDataPlot == nullptr) {
        liveDataPlot = m_plotManager->createPlot(liveSlot, configFromStore(liveSlot, defaultLiveConfig()), 400, 300);
        accelGraph = m_plotManager->createPlot(accelSlot, configFromStore(accelSlot, defaultAccelConfig()));
        gyroGraph = m_plotManager->createPlot(gyroSlot, configFromStore(gyroSlot, defaultGyroConfig()));
        return;
    }

    if (accelGraph == nullptr || gyroGraph == nullptr) {
        return;
    }

    m_plotManager->applyConfig(*liveDataPlot, configFromStore(liveSlot, defaultLiveConfig()));
    m_plotManager->applyConfig(*accelGraph, configFromStore(accelSlot, defaultAccelConfig()));
    m_plotManager->applyConfig(*gyroGraph, configFromStore(gyroSlot, defaultGyroConfig()));
}

QList<QCustomPlot *> ConnectionPage::allPlots() const {
    return QList<QCustomPlot *>{ liveDataPlot, accelGraph, gyroGraph };
}

void ConnectionPage::setupComboBox() {
    portList = new QComboBox();
    baudRate = new QComboBox();
    baudRate->addItem(QStringLiteral("115200"));
    baudRate->addItem(QStringLiteral("960000"));
}

void ConnectionPage::setupButtonsOnPage() {
    btnConnectPort = new QPushButton(QStringLiteral("Connect to port"));
    btnClosePort = new QPushButton(QStringLiteral("Disconnect from port"));
    updatePortList = new QPushButton(QStringLiteral("Update ports"));
}

void ConnectionPage::setupTrackingWidget() {
    trackingBlock = new QGroupBox(QStringLiteral("Tracked messages"));
    trackingList = new QListWidget();
    trackingList->setToolTip(
        QStringLiteral("Only checked messages are plotted and passed to the recorder"));
    trackingList->setMaximumHeight(150);

    auto *layout = new QVBoxLayout(trackingBlock);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->addWidget(trackingList);
}

void ConnectionPage::setupRecordedDataWidget() {
    recordedDataGroup = new QGroupBox(QStringLiteral("Recorded Data"));
    recordedDataStack = new QStackedWidget();

    noneModeLabel = new QLabel(QStringLiteral("Recording disabled"));
    noneModeLabel->setAlignment(Qt::AlignCenter);
    noneModeLabel->setStyleSheet(QStringLiteral("color: gray;"));

    onceStatusLabel = new QLabel(QStringLiteral("Buffer: 0 samples"));
    btnSaveOnce = new QPushButton(QStringLiteral("Save"));
    btnDiscardOnce = new QPushButton(QStringLiteral("Discard"));
    btnPrevGesture = new QPushButton(QStringLiteral("<"));
    btnNextGesture = new QPushButton(QStringLiteral(">"));
    listIndexLabel = new QLabel(QStringLiteral("0 / 0"));
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
    infoGroup = new QGroupBox(QStringLiteral("Sample Info"));

    QLabel *lengthCaption = new QLabel(QStringLiteral("Length:"));
    QLabel *freqCaption = new QLabel(QStringLiteral("Freq:"));
    QLabel *samplesCaption = new QLabel(QStringLiteral("Samples:"));
    QLabel *flagsCaption = new QLabel(QStringLiteral("Flags (start/end):"));

    infoLengthLabel = new QLabel(QStringLiteral("-- s"));
    infoFreqLabel = new QLabel(QStringLiteral("-- Hz"));
    infoSamplesLabel = new QLabel(QStringLiteral("--"));
    flagsLabel = new QLabel(QStringLiteral("-- / --"));

    QGridLayout *infoGridLayout = new QGridLayout(infoGroup);
    infoGridLayout->setContentsMargins(6, 6, 6, 6);
    infoGridLayout->addWidget(lengthCaption, 0, 0);
    infoGridLayout->addWidget(infoLengthLabel, 0, 1);
    infoGridLayout->addWidget(freqCaption, 1, 0);
    infoGridLayout->addWidget(infoFreqLabel, 1, 1);
    infoGridLayout->addWidget(samplesCaption, 2, 0);
    infoGridLayout->addWidget(infoSamplesLabel, 2, 1);
    infoGridLayout->addWidget(flagsCaption, 3, 0);
    infoGridLayout->addWidget(flagsLabel, 3, 1);
    infoGridLayout->setColumnStretch(2, 1);

    btnPrevGesture->setEnabled(false);
    btnNextGesture->setEnabled(false);
    btnSaveOnce->setEnabled(false);
    btnDiscardOnce->setEnabled(false);
}

void ConnectionPage::setupFlagMarkingWidget() {
    flagMarkingBlock = new QGroupBox(QStringLiteral("Flags marking block"));

    btnFlagMarking = new QPushButton(QStringLiteral("Marking file"));
    btnFlagAccept = new QPushButton(QStringLiteral("Accept"));
    btnFlagDeny = new QPushButton(QStringLiteral("Deny"));
    btnFlagMarking->setFixedWidth(100);
    btnFlagAccept->setFixedWidth(100);
    btnFlagDeny->setFixedWidth(100);
    flagMarkingLabel = new QLabel(QStringLiteral("0 / 0"));

    QWidget *markingPage = new QWidget();
    QHBoxLayout *markingLayout = new QHBoxLayout(markingPage);
    markingLayout->setContentsMargins(0, 0, 0, 0);
    markingLayout->addWidget(btnFlagMarking);
    markingLayout->addStretch();
    markingLayout->addWidget(flagMarkingLabel);
    markingLayout->addStretch();
    markingLayout->addWidget(btnFlagAccept);
    markingLayout->addWidget(btnFlagDeny);

    QVBoxLayout *rightDownMarkingLayout = new QVBoxLayout(flagMarkingBlock);
    rightDownMarkingLayout->setContentsMargins(6, 6, 6, 6);
    rightDownMarkingLayout->addWidget(markingPage);
}

void ConnectionPage::setupTrimBlockWidget() {
    trimBlock = new QGroupBox(QStringLiteral("Trim block"));

    btnTrimStart = new QPushButton(QStringLiteral("Use trim"));
    btnTrimAccept = new QPushButton(QStringLiteral("Accept"));
    btnTrimDeny = new QPushButton(QStringLiteral("Deny"));
    btnTrimStart->setFixedWidth(100);
    btnTrimAccept->setFixedWidth(100);
    btnTrimDeny->setFixedWidth(100);
    btnTrimStart->setEnabled(false);
    btnTrimAccept->setEnabled(false);
    btnTrimDeny->setEnabled(false);
    recordTimeTrim = new QLabel(QStringLiteral("0 / 0"));

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
    recordingMethod->addItem(QStringLiteral("None"));
    recordingMethod->addItem(QStringLiteral("Once method"));
    recordingMethod->addItem(QStringLiteral("List method"));

    motionType = new QComboBox();
    motionType->addItem(QStringLiteral("DoubleTap"), static_cast<int>(MotionType::DoubleTap));
    motionType->addItem(QStringLiteral("SwipeLeft"), static_cast<int>(MotionType::SwipeLeft));
    motionType->addItem(QStringLiteral("SwipeRight"), static_cast<int>(MotionType::SwipeRight));
    motionType->addItem(QStringLiteral("SwipeUp"), static_cast<int>(MotionType::SwipeUp));
    motionType->addItem(QStringLiteral("SwipeDown"), static_cast<int>(MotionType::SwipeDown));
    motionType->addItem(QStringLiteral("CircleCW"), static_cast<int>(MotionType::CircleCW));
    motionType->addItem(QStringLiteral("CircleCCW"), static_cast<int>(MotionType::CircleCCW));
    motionType->addItem(QStringLiteral("Shake"), static_cast<int>(MotionType::Shake));
    motionType->addItem(QStringLiteral("NormalHandMovement"), static_cast<int>(MotionType::NormalHandMovement));
    motionType->addItem(QStringLiteral("Walking"), static_cast<int>(MotionType::Walking));
    motionType->addItem(QStringLiteral("Unlabeled"), static_cast<int>(MotionType::Unlabeled));
    motionType->addItem(QStringLiteral("Unknown"), static_cast<int>(MotionType::Unknown));

    targetDbCombo = new QComboBox();
    databaseBlock = new QGroupBox(QStringLiteral("DB Block"));

    QWidget *dbBlock = new QWidget();

    QHBoxLayout *methodOfRecordLayout = new QHBoxLayout();
    methodOfRecordLayout->setContentsMargins(0, 0, 0, 0);
    methodOfRecordLayout->addWidget(new QLabel(QStringLiteral("Choose method record: ")));
    methodOfRecordLayout->addWidget(recordingMethod);
    methodOfRecordLayout->addStretch();

    QHBoxLayout *selectMovementLayout = new QHBoxLayout();
    selectMovementLayout->addWidget(new QLabel(QStringLiteral("Select movement")));
    selectMovementLayout->addWidget(motionType);
    selectMovementLayout->addStretch();

    QHBoxLayout *selectDataBaseLayout = new QHBoxLayout();
    selectDataBaseLayout->addWidget(new QLabel(QStringLiteral("Select database for save records")));
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
    portSettingsLayout->addWidget(new QLabel(QStringLiteral("Select port:")));
    portSettingsLayout->addWidget(portList);
    portSettingsLayout->addWidget(new QLabel(QStringLiteral("  Switch baud rate:")));
    portSettingsLayout->addWidget(baudRate);

    portButtonsLayout->addWidget(btnConnectPort);
    portButtonsLayout->addWidget(btnClosePort);

    settingsLayoutPort->addLayout(portSettingsLayout);
    settingsLayoutPort->addLayout(portButtonsLayout);
}

// ======================================================================
//  Разметка страницы
// ======================================================================

void ConnectionPage::createLayouts() {
    setupPortSettingsLayout();

    leftLayout->addLayout(settingsLayoutPort);
    leftLayout->addWidget(trackingBlock);
    leftLayout->addWidget(liveDataPlot, 1);
    leftLayout->addStretch(0);

    rightLayout->addWidget(gyroGraph, 1);
    rightLayout->addWidget(accelGraph, 1);

    QGroupBox *markingTrimBlockWidget = new QGroupBox(QStringLiteral("Edit graph"));
    QVBoxLayout *markingTrimBlockLayout = new QVBoxLayout(markingTrimBlockWidget);
    markingTrimBlockLayout->addWidget(trimBlock);
    markingTrimBlockLayout->addWidget(flagMarkingBlock);

    rightLayout->addLayout(rightUnderGraphLayout);
    rightLayout->addStretch();

    rightUnderGraphLayout->addWidget(databaseBlock, 0, 0);
    rightUnderGraphLayout->addWidget(markingTrimBlockWidget, 0, 1);
    rightUnderGraphLayout->addWidget(infoGroup, 1, 0);
    rightUnderGraphLayout->addWidget(recordedDataGroup, 1, 1);

    leftWidget->setLayout(leftLayout);
    rightWidget->setLayout(rightLayout);

    mainSplitter->addWidget(leftWidget);
    mainSplitter->addWidget(rightWidget);

    mainLayout->addWidget(mainSplitter);
    setLayout(mainLayout);
}

void ConnectionPage::connectSignals() {
    connect(updatePortList, &QPushButton::clicked, this, &ConnectionPage::updatePortsClicked);
    connect(btnConnectPort, &QPushButton::clicked, this, &ConnectionPage::connectPortClicked);
    connect(btnClosePort, &QPushButton::clicked, this, &ConnectionPage::closePortClicked);
    connect(btnClosePort, &QPushButton::clicked, this, &ConnectionPage::clearAllGraphs);

    connect(targetDbCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        emit targetDatabaseChanged(targetDbCombo->currentText());
    });
    connect(recordingMethod, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) { emit targetMethodChanged(recordingMethod->currentText()); });
    connect(motionType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        emit targetMotionTypeChanged(static_cast<MotionType>(motionType->itemData(index).toInt()));
    });

    connect(recordingMethod, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        recordedDataStack->setCurrentIndex(index == 0 ? 0 : 1);

        const bool isOnce = (index == 1);
        btnPrevGesture->setEnabled(index == 2);
        btnNextGesture->setEnabled(index == 2);
        listIndexLabel->setVisible(index == 2);

        WorkMode mode = WorkMode::WORK_LIST;
        if (index == 0) {
            mode = WorkMode::WORK_NONE;
        } else if (index == 1) {
            mode = WorkMode::WORK_ONCE;
        }
        setWorkMode(mode);

        btnSaveOnce->setEnabled(false);
        btnDiscardOnce->setEnabled(false);

        btnTrimStart->setEnabled(isOnce);
        resetTrim();
    });

    connect(btnSaveOnce, &QPushButton::clicked, this, &ConnectionPage::onSaveOnceClicked);
    connect(btnDiscardOnce, &QPushButton::clicked, this, [this]() {
        emit discardOnceRequested();
        btnSaveOnce->setEnabled(false);
        btnDiscardOnce->setEnabled(false);
        onceStatusLabel->setText(QStringLiteral("Buffer: 0 samples"));
        resetTrim();
    });

    connect(btnPrevGesture, &QPushButton::clicked, this, &ConnectionPage::prevGestureRequested);
    connect(btnNextGesture, &QPushButton::clicked, this, &ConnectionPage::nextGestureRequested);
    connect(btnFlagMarking, &QPushButton::clicked, this, &ConnectionPage::onFlagMarkingClicked);
    connect(btnFlagAccept, &QPushButton::clicked, this, &ConnectionPage::onFlagMarkingAcceptClicked);
    connect(btnFlagDeny, &QPushButton::clicked, this, &ConnectionPage::onFlagMarkingDenyClicked);
    connect(btnTrimStart, &QPushButton::clicked, this, &ConnectionPage::onTrimButtonClicked);
    connect(btnTrimAccept, &QPushButton::clicked, this, &ConnectionPage::onTrimAccept);
    connect(btnTrimDeny, &QPushButton::clicked, this, &ConnectionPage::onTrimDeny);

    for (QCustomPlot *plot : allPlots()) {
        connect(plot, &QCustomPlot::mousePress, this, [this, plot](QMouseEvent *e) {
            onGraphMousePress(e, plot);
        });
        connect(plot, &QCustomPlot::mouseMove, this, [this, plot](QMouseEvent *e) {
            onGraphMouseMove(e, plot);
        });
        connect(plot, &QCustomPlot::mouseRelease, this, [this, plot](QMouseEvent *e) {
            onGraphMouseRelease(e, plot);
        });
    }

    connect(trackingList, &QListWidget::itemChanged, this, &ConnectionPage::onTrackItemChanged);
    if (m_plotManager != nullptr) {
        connect(m_plotManager, &PlotManager::plotConfigured, this, &ConnectionPage::onPlotConfigured);
    }
}

// ======================================================================
//  Схемы и отслеживание
// ======================================================================

void ConnectionPage::setSchemas(const QList<DataSchema> &schemas) {
    m_schemas = schemas;
    if (m_plotManager != nullptr) {
        m_plotManager->setSchemas(schemas);
    }

    const bool blocked = trackingList->blockSignals(true);
    QStringList checked;
    for (int i = 0; i < trackingList->count(); ++i) {
        if (trackingList->item(i)->checkState() == Qt::Checked) {
            checked << trackingList->item(i)->data(Qt::UserRole).toString();
        }
    }
    trackingList->clear();

    // По умолчанию отслеживается встроенная IMU-схема.
    const QString imu = builtin(BuiltinSchemas::imuName);
    if (checked.isEmpty() && schemas.isEmpty() == false) {
        checked << imu;
    }

    for (const DataSchema &schema : schemas) {
        auto *item = new QListWidgetItem(
            QStringLiteral("%1 (id %2, %3 B)").arg(schema.name).arg(schema.typeId).arg(schema.totalSize()));
        item->setData(Qt::UserRole, schema.name);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(checked.contains(schema.name) ? Qt::Checked : Qt::Unchecked);
        trackingList->addItem(item);
    }
    trackingList->blockSignals(blocked);

    applyAllPlotConfigs();
    emit trackedSchemasChanged(checked);
}

bool ConnectionPage::isSchemaTracked(const QString &schemaName) const {
    for (int i = 0; i < trackingList->count(); ++i) {
        QListWidgetItem *item = trackingList->item(i);
        if (item->data(Qt::UserRole).toString() == schemaName) {
            return item->checkState() == Qt::Checked;
        }
    }
    return false;
}

void ConnectionPage::onTrackItemChanged(QListWidgetItem *) {
    QStringList checked;
    for (int i = 0; i < trackingList->count(); ++i) {
        if (trackingList->item(i)->checkState() == Qt::Checked) {
            checked << trackingList->item(i)->data(Qt::UserRole).toString();
        }
    }
    emit trackedSchemasChanged(checked);
}

void ConnectionPage::onPlotConfigured(const QString &slotId, const PlotConfig &config) {
    // Новые наборы полей несовместимы со старыми точками, поэтому буфер
    // переконфигурированного слота сбрасываем. Остальные слоты той же
    // схемы не трогаем - они продолжают принимать данные.
    if (m_plotManager == nullptr) {
        return;
    }
    const QList<QCustomPlot *> plots = allPlots();
    if (plots.contains(nullptr)) {
        return;
    }
    const int index = plots.indexOf(plotForSlot(slotId));
    if (index < 0) {
        return;
    }

    QCustomPlot *plot = plots.at(index);
    if (m_plotManager->isEmpty(*plot)) {
        return;
    }
    emit logMessage(LogLevel::Debug,
                    QStringLiteral("Graph \"%1\" reconfigured to \"%2\", data cleared")
                        .arg(slotId, config.schemaName));
    m_plotManager->clearPlot(*plot);
}

// ======================================================================
//  Приём кадров
// ======================================================================

void ConnectionPage::onFrame(const DataFrame &frame) {
    if (isSchemaTracked(frame.schemaName()) == false) {
        return;
    }
    if (m_workMode == WorkMode::WORK_LIST) {
        return;
    }

    updateTime(frame);

    bool recording = false;
    frame.isRecording(&recording);

    if (recording && m_lastRecordingState == false) {
        clearRecordPlots();
        addSeparator(m_tempTime);
        m_timeStartSnaphots = m_tempTime;
        resetCountSnapshots();
        resetTrim();
    }

    if (m_plotManager != nullptr) {
        m_plotManager->appendFrame(*liveDataPlot, frame);
        if (recording) {
            m_plotManager->appendFrame(*accelGraph, frame);
            m_plotManager->appendFrame(*gyroGraph, frame);
            m_plotManager->refreshLive(*liveDataPlot);
            m_plotManager->refreshLive(*accelGraph);
            m_plotManager->refreshLive(*gyroGraph);
        } else {
            m_plotManager->refreshLive(*liveDataPlot);
        }
    }

    if (recording) {
        ++m_snaphots;
    }

    if (m_lastRecordingState && recording == false) {
        addSeparator(m_tempTime);
        const double length = m_tempTime - m_timeStartSnaphots;
        const double freq = length > 0 ? m_snaphots / length : 0.0;
        updateInfoBox(length, freq, m_snaphots);
    }

    m_lastRecordingState = recording;
}

void ConnectionPage::onSegmentBoundary(double absoluteX) {
    if (absoluteX > 0) {
        addSeparator(absoluteX);
    }
}

void ConnectionPage::updateTime(const DataFrame &frame) {
    bool ok = false;
    const double seconds = frame.timeSeconds(&ok);
    m_lastAbsX = ok ? seconds : m_lastAbsX;
    m_tempTime = m_lastAbsX;
}

// ======================================================================
//  Маркеры
// ======================================================================

void ConnectionPage::addSeparator(double absoluteX) {
    for (QCustomPlot *plot : allPlots()) {
        addSeparatorTo(plot, absoluteX);
    }
}

void ConnectionPage::addSeparatorTo(QCustomPlot *plot, double absoluteX) {
    if (m_plotManager != nullptr && plot != nullptr) {
        m_plotManager->addMarker(*plot, m_plotManager->absoluteToPlotX(*plot, absoluteX),
                                 MarkerType::Separator);
    }
}

void ConnectionPage::clearSeparators() {
    if (m_plotManager == nullptr) {
        return;
    }
    for (QCustomPlot *plot : allPlots()) {
        m_plotManager->clearMarkers(*plot, MarkerType::Separator);
    }
}

void ConnectionPage::addTrimLine(double absoluteX, bool isStart) {
    if (m_plotManager == nullptr) {
        return;
    }
    const MarkerType type = isStart ? MarkerType::TrimStart : MarkerType::TrimEnd;
    m_plotManager->clearMarkers(*accelGraph, type);
    m_plotManager->clearMarkers(*gyroGraph, type);
    m_plotManager->addMarker(*accelGraph, m_plotManager->absoluteToPlotX(*accelGraph, absoluteX), type);
    m_plotManager->addMarker(*gyroGraph, m_plotManager->absoluteToPlotX(*gyroGraph, absoluteX), type);
}

void ConnectionPage::updateTrimLine(double absoluteX, bool isStart) {
    if (m_plotManager == nullptr) {
        return;
    }
    const MarkerType type = isStart ? MarkerType::TrimStart : MarkerType::TrimEnd;
    m_plotManager->setMarkerX(*accelGraph, type, m_plotManager->absoluteToPlotX(*accelGraph, absoluteX));
    m_plotManager->setMarkerX(*gyroGraph, type, m_plotManager->absoluteToPlotX(*gyroGraph, absoluteX));
}

void ConnectionPage::clearTrimSeparators() {
    if (m_plotManager == nullptr) {
        return;
    }
    m_plotManager->clearMarkers(*accelGraph, MarkerType::TrimStart);
    m_plotManager->clearMarkers(*accelGraph, MarkerType::TrimEnd);
    m_plotManager->clearMarkers(*gyroGraph, MarkerType::TrimStart);
    m_plotManager->clearMarkers(*gyroGraph, MarkerType::TrimEnd);
}

void ConnectionPage::addFlagLine(double absoluteX, bool isStart) {
    if (m_plotManager == nullptr) {
        return;
    }
    const MarkerType type = isStart ? MarkerType::FlagStart : MarkerType::FlagEnd;
    m_plotManager->clearMarkers(*accelGraph, type);
    m_plotManager->clearMarkers(*gyroGraph, type);
    m_plotManager->addMarker(*accelGraph, m_plotManager->absoluteToPlotX(*accelGraph, absoluteX), type);
    m_plotManager->addMarker(*gyroGraph, m_plotManager->absoluteToPlotX(*gyroGraph, absoluteX), type);
}

void ConnectionPage::updateFlagLine(double absoluteX, bool isStart) {
    if (m_plotManager == nullptr) {
        return;
    }
    const MarkerType type = isStart ? MarkerType::FlagStart : MarkerType::FlagEnd;
    m_plotManager->setMarkerX(*accelGraph, type, m_plotManager->absoluteToPlotX(*accelGraph, absoluteX));
    m_plotManager->setMarkerX(*gyroGraph, type, m_plotManager->absoluteToPlotX(*gyroGraph, absoluteX));
}

void ConnectionPage::clearFlagMarkingSeparators() {
    if (m_plotManager == nullptr) {
        return;
    }
    m_plotManager->clearMarkers(*accelGraph, MarkerType::FlagStart);
    m_plotManager->clearMarkers(*accelGraph, MarkerType::FlagEnd);
    m_plotManager->clearMarkers(*gyroGraph, MarkerType::FlagStart);
    m_plotManager->clearMarkers(*gyroGraph, MarkerType::FlagEnd);
}

void ConnectionPage::setTrimInteractionEnabled(bool on) {
    if (m_plotManager == nullptr) {
        return;
    }
    if (on) {
        m_plotManager->setRangeDragEnabled(*accelGraph, false);
        m_plotManager->setRangeDragEnabled(*gyroGraph, false);
    } else {
        m_plotManager->setRangeDragEnabled(*accelGraph, m_rangeDragAccel);
        m_plotManager->setRangeDragEnabled(*gyroGraph, m_rangeDragGyro);
    }
}

void ConnectionPage::setFlagMarkingInteractionEnabled(bool on) {
    if (m_plotManager == nullptr) {
        return;
    }
    if (on) {
        m_plotManager->setRangeDragEnabled(*accelGraph, false);
        m_plotManager->setRangeDragEnabled(*gyroGraph, false);
    } else {
        m_plotManager->setRangeDragEnabled(*accelGraph, m_flagMarkingDragAccel);
        m_plotManager->setRangeDragEnabled(*gyroGraph, m_flagMarkingDragGyro);
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
    if (recordTimeTrim) {
        recordTimeTrim->setText(QStringLiteral("0 / 0"));
    }
}

void ConnectionPage::resetFlagMarking() {
    clearFlagMarkingSeparators();
    m_startFlagSec = m_endFlagSec = -1;
    m_flagState = FlagState::Off;
    m_flagMarkingDragging = false;
    btnFlagAccept->setEnabled(false);
    btnFlagDeny->setEnabled(false);
    setFlagMarkingInteractionEnabled(false);
    if (flagMarkingLabel) {
        flagMarkingLabel->setText(QStringLiteral("0 / 0"));
    }
}

double ConnectionPage::clampToData(QCustomPlot &plot, double absoluteX) const {
    if (m_motionSaved.isEmpty() || m_plotManager == nullptr) {
        return absoluteX;
    }
    double lo = 0.0;
    double hi = 0.0;
    if (m_plotManager->dataXRange(plot, &lo, &hi) == false) {
        return absoluteX;
    }
    return qBound(m_plotManager->plotXToAbsolute(plot, lo), absoluteX,
                  m_plotManager->plotXToAbsolute(plot, hi));
}

void ConnectionPage::indicesForRange(double loSec, double hiSec, int *startIdx, int *endIdx) const {
    int start = 0;
    int end = 0;
    for (int i = 0; i < m_motionSaved.size(); ++i) {
        bool ok = false;
        const double t = m_motionSaved.at(i).timeSeconds(&ok);
        if (ok == false) {
            continue;
        }
        if (t >= loSec) {
            start = i;
            break;
        }
    }
    for (int i = m_motionSaved.size() - 1; i >= 0; --i) {
        bool ok = false;
        const double t = m_motionSaved.at(i).timeSeconds(&ok);
        if (ok && t <= hiSec) {
            end = i;
            break;
        }
    }
    if (startIdx) *startIdx = start;
    if (endIdx) *endIdx = end;
}

void ConnectionPage::sizeListUpdate(int size) {
    m_sizeListBuffer = size;
    m_currentKey = m_sizeListBuffer;
    listIndexLabel->setText(QStringLiteral("%1 / %1").arg(m_sizeListBuffer));
}

void ConnectionPage::indexListSelected(int index) {
    listIndexLabel->setText(QStringLiteral("%1 / %2").arg(index).arg(m_sizeListBuffer));
    m_currentKey = index;
}

// ======================================================================
//  Работа с мышью: trim и флаги
// ======================================================================

void ConnectionPage::onTrimButtonClicked() {
    if (btnTrimStart->isEnabled() == false) {
        return;
    }

    if (m_trimState == TrimState::Off) {
        if (m_plotManager != nullptr) {
            m_rangeDragAccel = m_plotManager->isRangeDragEnabled(*accelGraph);
            m_rangeDragGyro = m_plotManager->isRangeDragEnabled(*gyroGraph);
        }
        clearTrimSeparators();
        m_trimStartSec = m_trimEndSec = -1;
        m_trimState = TrimState::AwaitStart;
        btnTrimAccept->setEnabled(false);
        btnTrimDeny->setEnabled(false);
        setTrimInteractionEnabled(true);
        if (recordTimeTrim) {
            recordTimeTrim->setText(QStringLiteral("click start / -"));
        }
    } else {
        resetTrim();
    }
}

void ConnectionPage::onGraphMousePress(QMouseEvent *event, QCustomPlot *plot) {
    if (m_trimState == TrimState::Off && m_flagState == FlagState::Off) {
        return;
    }

    const double sec = clampToData(*plot, m_plotManager->plotXToAbsolute(*plot,
                                                                          plot->xAxis->pixelToCoord(event->pos().x())));

    if (m_trimState == TrimState::AwaitStart) {
        m_trimStartSec = sec;
        addTrimLine(sec, true);
        m_trimState = TrimState::AwaitEnd;
        if (recordTimeTrim) {
            recordTimeTrim->setText(QStringLiteral("%1 / -").arg(sec, 0, 'f', 3));
        }
        return;
    }
    if (m_trimState == TrimState::AwaitEnd) {
        m_trimEndSec = sec;
        addTrimLine(sec, false);
        m_trimState = TrimState::Adjust;
        btnTrimAccept->setEnabled(true);
        btnTrimDeny->setEnabled(true);
        if (recordTimeTrim) {
            recordTimeTrim->setText(QStringLiteral("%1 / %2")
                                        .arg(m_trimStartSec, 0, 'f', 3)
                                        .arg(m_trimEndSec, 0, 'f', 3));
        }
        return;
    }
    if (m_trimState == TrimState::Adjust) {
        const double px = event->pos().x();
        if (m_trimStartSec >= 0
            && qAbs(plot->xAxis->coordToPixel(m_plotManager->absoluteToPlotX(*plot, m_trimStartSec)) - px)
                   <= TrimDragThresholdPx) {
            m_trimDragging = true;
            m_trimDragIsStart = true;
            return;
        }
        if (m_trimEndSec >= 0
            && qAbs(plot->xAxis->coordToPixel(m_plotManager->absoluteToPlotX(*plot, m_trimEndSec)) - px)
                   <= TrimDragThresholdPx) {
            m_trimDragging = true;
            m_trimDragIsStart = false;
        }
        return;
    }

    if (m_flagState == FlagState::AwaitStart) {
        m_startFlagSec = sec;
        addFlagLine(sec, true);
        m_flagState = FlagState::AwaitEnd;
        if (flagMarkingLabel) {
            flagMarkingLabel->setText(QStringLiteral("%1 / -").arg(sec, 0, 'f', 3));
        }
        return;
    }
    if (m_flagState == FlagState::AwaitEnd) {
        m_endFlagSec = sec;
        addFlagLine(sec, false);
        m_flagState = FlagState::Adjust;
        btnFlagAccept->setEnabled(true);
        btnFlagDeny->setEnabled(true);
        if (flagMarkingLabel) {
            flagMarkingLabel->setText(QStringLiteral("%1 / %2")
                                          .arg(m_startFlagSec, 0, 'f', 3)
                                          .arg(m_endFlagSec, 0, 'f', 3));
        }
        return;
    }
    if (m_flagState == FlagState::Adjust) {
        const double px = event->pos().x();
        if (m_startFlagSec >= 0
            && qAbs(plot->xAxis->coordToPixel(m_plotManager->absoluteToPlotX(*plot, m_startFlagSec)) - px)
                   <= flagMarkingDragThresholdPx) {
            m_flagMarkingDragging = true;
            m_flagMarkingDragIsStart = true;
            return;
        }
        if (m_endFlagSec >= 0
            && qAbs(plot->xAxis->coordToPixel(m_plotManager->absoluteToPlotX(*plot, m_endFlagSec)) - px)
                   <= flagMarkingDragThresholdPx) {
            m_flagMarkingDragging = true;
            m_flagMarkingDragIsStart = false;
        }
    }
}

void ConnectionPage::onGraphMouseMove(QMouseEvent *event, QCustomPlot *plot) {
    if (m_trimDragging == false && m_flagMarkingDragging == false) {
        return;
    }
    const double sec = clampToData(*plot, m_plotManager->plotXToAbsolute(*plot,
                                                                          plot->xAxis->pixelToCoord(event->pos().x())));

    if (m_trimDragging) {
        if (m_trimDragIsStart) {
            m_trimStartSec = sec;
        } else {
            m_trimEndSec = sec;
        }
        updateTrimLine(sec, m_trimDragIsStart);
        if (recordTimeTrim) {
            recordTimeTrim->setText(QStringLiteral("%1 / %2")
                                        .arg(m_trimStartSec, 0, 'f', 3)
                                        .arg(m_trimEndSec, 0, 'f', 3));
        }
    }

    if (m_flagMarkingDragging) {
        if (m_flagMarkingDragIsStart) {
            m_startFlagSec = sec;
        } else {
            m_endFlagSec = sec;
        }
        updateFlagLine(sec, m_flagMarkingDragIsStart);
        if (flagMarkingLabel) {
            flagMarkingLabel->setText(QStringLiteral("%1 / %2")
                                          .arg(m_startFlagSec, 0, 'f', 3)
                                          .arg(m_endFlagSec, 0, 'f', 3));
        }
    }
}

void ConnectionPage::onGraphMouseRelease(QMouseEvent *event, QCustomPlot *plot) {
    Q_UNUSED(event);
    Q_UNUSED(plot);
    m_trimDragging = false;
    m_flagMarkingDragging = false;
}

void ConnectionPage::onTrimAccept() {
    if (m_trimStartSec < 0 || m_trimEndSec < 0) {
        return;
    }

    const double lo = qMin(m_trimStartSec, m_trimEndSec);
    const double hi = qMax(m_trimStartSec, m_trimEndSec);

    if (m_motionSaved.isEmpty() == false) {
        QVector<DataFrame> trimmed;
        for (const DataFrame &frame : qAsConst(m_motionSaved)) {
            bool ok = false;
            const double t = frame.timeSeconds(&ok);
            if (ok && t >= lo && t <= hi) {
                trimmed.append(frame);
            }
        }
        m_motionSaved = trimmed;
        showSavedGraph(m_motionSaved);
        drawSavedSegmentSeparators();
        onceStatusLabel->setText(QStringLiteral("Buffer: %1 samples").arg(m_motionSaved.size()));
        btnSaveOnce->setEnabled(m_motionSaved.isEmpty() == false);
        btnDiscardOnce->setEnabled(m_motionSaved.isEmpty() == false);
    }

    emit trimRequested(lo, hi);
    resetTrim();
}

void ConnectionPage::onTrimDeny() {
    resetTrim();
}

void ConnectionPage::onFlagMarkingClicked() {
    if (m_flagState == FlagState::Off) {
        if (m_plotManager != nullptr) {
            m_flagMarkingDragAccel = m_plotManager->isRangeDragEnabled(*accelGraph);
            m_flagMarkingDragGyro = m_plotManager->isRangeDragEnabled(*gyroGraph);
        }
        clearFlagMarkingSeparators();
        m_startFlagSec = m_endFlagSec = -1;
        m_flagState = FlagState::AwaitStart;
        btnFlagAccept->setEnabled(false);
        btnFlagDeny->setEnabled(false);
        setFlagMarkingInteractionEnabled(true);
        if (flagMarkingLabel) {
            flagMarkingLabel->setText(QStringLiteral("click start / -"));
        }
    } else {
        resetFlagMarking();
    }
}

void ConnectionPage::onFlagMarkingAcceptClicked() {
    if (m_startFlagSec < 0 || m_endFlagSec < 0) {
        return;
    }

    const double lo = qMin(m_startFlagSec, m_endFlagSec);
    const double hi = qMax(m_startFlagSec, m_endFlagSec);

    int startIdx = 0;
    int endIdx = 0;
    indicesForRange(lo, hi, &startIdx, &endIdx);

    if (flagMarkingLabel) {
        flagMarkingLabel->setText(QStringLiteral("%1 / %2").arg(startIdx).arg(endIdx));
    }
    if (flagsLabel) {
        flagsLabel->setText(QStringLiteral("%1 / %2").arg(startIdx).arg(endIdx));
    }

    emit flagMarkingRequested(startIdx, endIdx);

    m_flagState = FlagState::Off;
    m_flagMarkingDragging = false;
    btnFlagAccept->setEnabled(false);
    btnFlagDeny->setEnabled(false);
    setFlagMarkingInteractionEnabled(false);
}

void ConnectionPage::onFlagMarkingDenyClicked() {
    resetFlagMarking();
}

// ======================================================================
//  Очистка и сохранённый жест
// ======================================================================

void ConnectionPage::clearRecordPlots() {
    if (m_plotManager == nullptr) {
        return;
    }
    m_plotManager->clearPlot(*accelGraph);
    m_plotManager->clearPlot(*gyroGraph);
    clearSeparators();
}

void ConnectionPage::clearAllGraphs() {
    if (m_plotManager == nullptr) {
        return;
    }
    for (QCustomPlot *plot : allPlots()) {
        m_plotManager->clearPlot(*plot);
    }
    resetTime();
    resetTrim();
    resetFlagMarking();
}

void ConnectionPage::resetTime() {
    m_tempTime = 0;
    m_lastAbsX = 0;
    m_lastRecordingState = false;
}

void ConnectionPage::updateInfoBox(double length, double freq, int samples) {
    emit logMessage(LogLevel::Info,
                    QStringLiteral("%1 packages arrived. Time motion: %2 sec. Freq(Hz): %3")
                        .arg(samples)
                        .arg(length)
                        .arg(freq));
    infoLengthLabel->setNum(length);
    infoFreqLabel->setNum(freq);
    infoSamplesLabel->setNum(samples);
}

void ConnectionPage::showSavedGraph(const QVector<DataFrame> &frames) {
    if (frames.isEmpty() || m_plotManager == nullptr) {
        return;
    }

    clearAllGraphs();
    m_plotManager->paintFrames(*liveDataPlot, frames);
    m_plotManager->paintFrames(*accelGraph, frames);
    m_plotManager->paintFrames(*gyroGraph, frames);
}

void ConnectionPage::drawSavedSegmentSeparators() {
    if (m_motionSaved.isEmpty()) {
        return;
    }
    bool firstOk = false;
    bool lastOk = false;
    const double x0 = m_motionSaved.first().timeSeconds(&firstOk);
    const double x1 = m_motionSaved.last().timeSeconds(&lastOk);
    if (firstOk == false || lastOk == false) {
        return;
    }
    addSeparator(x0);
    addSeparator(x1);
}

void ConnectionPage::showSavedFrames(const QVector<DataFrame> &frames, int key) {
    if (frames.isEmpty()) {
        return;
    }

    if (m_workMode == WorkMode::WORK_ONCE) {
        m_motionSaved = frames;
        showSavedGraph(frames);
        drawSavedSegmentSeparators();
        btnSaveOnce->setEnabled(true);
        btnDiscardOnce->setEnabled(true);
        onceStatusLabel->setText(QStringLiteral("Buffer: %1 samples").arg(frames.size()));
        return;
    }

    if (m_workMode == WorkMode::WORK_LIST) {
        indexListSelected(key);
        m_motionSaved = frames;
        showSavedGraph(frames);
        drawSavedSegmentSeparators();
    }
}

void ConnectionPage::onSaveOnceClicked() {
    if (m_motionSaved.isEmpty()) {
        return;
    }

    bool firstOk = false;
    bool lastOk = false;
    const double first = m_motionSaved.first().timeSeconds(&firstOk);
    const double last = m_motionSaved.last().timeSeconds(&lastOk);
    const double duration = (firstOk && lastOk) ? (last - first) : 0.0;
    const int sampleCount = m_motionSaved.size();

    const QString message = QStringLiteral("Confirm saving gesture:\n\n"
                                           "Movement: %1\n"
                                           "Database: %2\n"
                                           "Time: %3 s\n"
                                           "Samples: %4")
                                .arg(motionType->currentText())
                                .arg(targetDbCombo->currentText())
                                .arg(duration, 0, 'f', 3)
                                .arg(sampleCount);

    const auto reply = QMessageBox::question(this, QStringLiteral("Save gesture"), message,
                                             QMessageBox::Save | QMessageBox::Cancel);
    if (reply != QMessageBox::Save) {
        return;
    }

    emit saveOnceRequested();

    m_motionSaved.clear();
    btnSaveOnce->setEnabled(false);
    btnDiscardOnce->setEnabled(false);
    onceStatusLabel->setText(QStringLiteral("Saved"));
}

// ======================================================================
//  Порты, базы, режим
// ======================================================================

void ConnectionPage::setPorts(const QStringList &ports) {
    portList->clear();
    for (const QString &port : ports) {
        portList->addItem(port);
    }
}

void ConnectionPage::setAvailableDatabases(const QStringList &databases) {
    const QString current = targetDbCombo->currentText();
    targetDbCombo->clear();
    targetDbCombo->addItems(databases);
    const int index = targetDbCombo->findText(current);
    if (index >= 0) {
        targetDbCombo->setCurrentIndex(index);
    }
}

PortConfig ConnectionPage::connectTo() {
    PortConfig config;
    config.name = portList->currentText();
    config.baud = baudRate->currentText().toInt();
    return config;
}

void ConnectionPage::setWorkMode(WorkMode &mode) {
    m_workMode = mode;
}

WorkMode ConnectionPage::getWorkMode() const {
    return m_workMode;
}
