#pragma once

#include <QList>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include "models/datapacket.h"
#include "models/dataschema.h"
#include "models/loglevel.h"
#include "models/MotionType.h"
#include "models/portconfig.h"
#include "modules/plot/plotconfig.h"

class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QComboBox;
class QSplitter;
class QCustomPlot;
class QMouseEvent;
class QLabel;
class QGroupBox;
class QStackedWidget;
class QGridLayout;
class QListWidget;
class QListWidgetItem;
class PlotManager;
class PlotConfigStore;
class SchemaStore;

// =================== Work modes:
// 1. None - live graph in real time, graphs draw the tracked message. Can't save motion
// 2. Once - live graph in real time, graphs draw the tracked message.
//                              Saved last motion in buffer. User can save and edit motion
// 3. List - live graph in real time, graphs draw the tracked message. Motions saved in buffer
//                                  User can switch, edit and saved  motion.

enum class WorkMode : int {
    WORK_NONE = 1,
    WORK_ONCE,
    WORK_LIST
};

// Страница подключения. Получает декодированные кадры любой схемы,
// показывает их на настраиваемых графиках (конфигурация - правый клик)
// и ведёт буфер записи жеста для встроенной схемы IMU.
class ConnectionPage : public QWidget
{
    Q_OBJECT

public:
    ConnectionPage(PlotManager *plotManager,
                   PlotConfigStore *plotConfigStore,
                   QWidget *parent = nullptr);

    // Схемы приходят из SchemaStore и обновляются после правки редактора.
    void setSchemas(const QList<DataSchema> &schemas);

    void setPorts(const QStringList &ports);
    void setAvailableDatabases(const QStringList &databases);

    // Живой поток: один кадр любой схемы.
    void onFrame(const DataFrame &frame);
    // Границы сегмента (кадр с ролью crc16).
    void onSegmentBoundary(double absoluteX);
    // Готовый жест из буфера записи.
    void showSavedFrames(const QVector<DataFrame> &frames, int key);

    PortConfig connectTo();

    void sizeListUpdate(int key);

    void setWorkMode(WorkMode &mode);
    WorkMode getWorkMode() const;

    bool isSchemaTracked(const QString &schemaName) const;

private slots:
    void onTrackItemChanged(QListWidgetItem *item);
    void onPlotConfigured(const QString &slotId, const PlotConfig &config);
    // График, закреплённый за идентификатором слота.
    QCustomPlot *plotForSlot(const QString &slotId);

signals:
    void updatePortsClicked();
    void connectPortClicked();
    void closePortClicked();

    void targetDatabaseChanged(const QString &dbName);
    void targetMethodChanged(const QString &MethodName);
    void targetMotionTypeChanged(const MotionType &type);
    // Схемы, отмеченные в панели отслеживания.
    void trackedSchemasChanged(const QStringList &schemaNames);

    void saveOnceRequested();
    void discardOnceRequested();
    void prevGestureRequested();
    void nextGestureRequested();
    void trimRequested(double loSec, double hiSec);
    void flagMarkingRequested(int startFlag, int endFlag);

    void logMessage(LogLevel level, const QString &text);

private:
    void createWidgets();
    void createLayouts();
    void connectSignals();

    // ============ Настройка виджетов

    void setupGraphs();
    void setupComboBox();
    void setupButtonsOnPage();
    void setupRecordedDataWidget();
    void setupInfoBlockWidget();
    void setupTrimBlockWidget();
    void setupDataBaseBlockWidget();
    void setupFlagMarkingWidget();
    void setupTrackingWidget();
    void setupPortSettingsLayout();

    // ============ Графики

    PlotConfig configFromStore(const QString &slotId, const PlotConfig &fallback) const;
    // Набор из трёх графиков по умолчанию для встроенной схемы IMU.
    PlotConfig defaultLiveConfig() const;
    PlotConfig defaultAccelConfig() const;
    PlotConfig defaultGyroConfig() const;
    void applyAllPlotConfigs();

    QList<QCustomPlot *> allPlots() const;
    void clearAllGraphs();
    void clearRecordPlots();
    void clearSeparators();
    void addSeparator(double absoluteX);
    void addSeparatorTo(QCustomPlot *plot, double absoluteX);
    void updateTime(const DataFrame &frame);
    void resetTime();
    void resetCountSnapshots() { m_snaphots = 0; }
    void updateInfoBox(double length, double freq, int samples);
    void onSaveOnceClicked();
    void showSavedGraph(const QVector<DataFrame> &frames);
    void drawSavedSegmentSeparators();

    // ============ Разметка отрезков (trim / flags)

    enum class FlagState { Off, AwaitStart, AwaitEnd, Adjust };
    void resetFlagMarking();
    void onFlagMarkingClicked();
    void onFlagMarkingAcceptClicked();
    void onFlagMarkingDenyClicked();
    void setFlagMarkingInteractionEnabled(bool on);
    void clearFlagMarkingSeparators();
    void addFlagLine(double absoluteX, bool isStart);
    void updateFlagLine(double absoluteX, bool isStart);

    enum class TrimState { Off, AwaitStart, AwaitEnd, Adjust };
    void onTrimButtonClicked();
    void onTrimAccept();
    void onTrimDeny();
    void onGraphMousePress(QMouseEvent *event, QCustomPlot *plot);
    void onGraphMouseMove(QMouseEvent *event, QCustomPlot *plot);
    void onGraphMouseRelease(QMouseEvent *event, QCustomPlot *plot);
    void addTrimLine(double absoluteX, bool isStart);
    void updateTrimLine(double absoluteX, bool isStart);
    void clearTrimSeparators();
    void resetTrim();
    void setTrimInteractionEnabled(bool on);
    // Ограничить координату по данным сохранённого жеста.
    double clampToData(QCustomPlot &plot, double absoluteX) const;
    // Индексы кадров, попадающих в отрезок абсолютного времени.
    void indicesForRange(double loSec, double hiSec, int *startIdx, int *endIdx) const;
    void indexListSelected(int index);

    // ============ Виджеты

    QWidget *leftWidget;
    QWidget *rightWidget;
    QSplitter *mainSplitter;
    QHBoxLayout *mainLayout;
    QVBoxLayout *leftLayout;
    QVBoxLayout *rightLayout;
    QGridLayout *rightUnderGraphLayout;

    QVBoxLayout *settingsLayoutLeft;
    QVBoxLayout *settingsLayoutPort;
    QHBoxLayout *portSettingsLayout;
    QHBoxLayout *portButtonsLayout;

    QComboBox *portList;
    QComboBox *baudRate;
    QPushButton *btnConnectPort;
    QPushButton *btnClosePort;
    QPushButton *updatePortList;

    QGroupBox *recordedDataGroup;
    QStackedWidget *recordedDataStack;
    QLabel *noneModeLabel;
    QLabel *onceStatusLabel;
    QPushButton *btnSaveOnce;
    QPushButton *btnDiscardOnce;
    QPushButton *btnPrevGesture;
    QPushButton *btnNextGesture;
    QLabel *listIndexLabel;

    QGroupBox *infoGroup;
    QLabel *infoLengthLabel;
    QLabel *infoFreqLabel;
    QLabel *infoSamplesLabel;
    QLabel *flagsLabel;

    QGroupBox *flagMarkingBlock;
    QPushButton *btnFlagMarking;
    QPushButton *btnFlagAccept;
    QPushButton *btnFlagDeny;
    QLabel *flagMarkingLabel;

    QGroupBox *trimBlock;
    QPushButton *btnTrimStart;
    QPushButton *btnTrimAccept;
    QPushButton *btnTrimDeny;
    QLabel *recordTimeTrim;

    QGroupBox *databaseBlock;
    QComboBox *targetDbCombo;
    QComboBox *recordingMethod;
    QComboBox *motionType;

    QGroupBox *trackingBlock;
    QListWidget *trackingList;

    // ============ Графики и состояние

    QCustomPlot *gyroGraph = nullptr;
    QCustomPlot *accelGraph = nullptr;
    QCustomPlot *liveDataPlot = nullptr;

    PlotManager *m_plotManager = nullptr;
    PlotConfigStore *m_plotConfigStore = nullptr;
    QList<DataSchema> m_schemas;

    double m_tempTime = 0;
    double m_lastAbsX = 0;
    bool m_lastRecordingState = false;
    int m_snaphots = 0;
    double m_timeStartSnaphots = 0;
    WorkMode m_workMode = WorkMode::WORK_NONE;
    QVector<DataFrame> m_motionSaved;
    int m_sizeListBuffer = 0;
    int m_currentKey = 0;

    double m_startFlagSec = -1;
    double m_endFlagSec = -1;
    FlagState m_flagState = FlagState::Off;
    bool m_flagMarkingDragging = false;
    bool m_flagMarkingDragIsStart = false;
    bool m_flagMarkingDragAccel = false;
    bool m_flagMarkingDragGyro = false;
    static constexpr int flagMarkingDragThresholdPx = 6;

    double m_trimStartSec = -1;
    double m_trimEndSec = -1;
    TrimState m_trimState = TrimState::Off;
    bool m_trimDragging = false;
    bool m_trimDragIsStart = false;
    bool m_rangeDragAccel = false;
    bool m_rangeDragGyro = false;
    static constexpr int TrimDragThresholdPx = 6;
};
