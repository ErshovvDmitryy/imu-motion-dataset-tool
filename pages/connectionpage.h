#pragma once

#include <QObject>
#include <QList>
#include <QWidget>
#include <QStringList>
#include <QVector>
#include <QMap>

#include "models/motionsample.h"
#include "models/MotionType.h"
#include "models/portconfig.h"
#include "models/loglevel.h"

class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QComboBox;
class QSplitter;
class QCustomPlot;
class QCPItemStraightLine;
class QMouseEvent;
class QLabel;
class QGroupBox;
class QStackedWidget;
class QGridLayout;

enum class WorkMode : int {
    WORK_NONE = 1,
    WORK_ONCE,
    WORK_LIST
};

class ConnectionPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectionPage(QWidget *parent = nullptr);

    void drawSeparator();
    void drawSavedSegmentSeparators();
    void updateStream(const MotionPacket &packet);
    void setPorts(const QStringList &ports);
    void setAvailableDatabases(const QStringList &databases);

    void incommingSegment(QVector<MotionSample> &receivedData);

    PortConfig connectTo();

    void setWorkMode(WorkMode &mode);
    WorkMode getWorkMode() const;

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

    void updateInfoBox(const float &infoLength, const float &infoFreqLabel, const int infoSamplesLabel);
    void updateSavedGraph(QVector<MotionSample> &motionSaved);

    void onSaveOnceClicked();

    enum class TrimState { Off, AwaitStart, AwaitEnd, Adjust };

    void onTrimButtonClicked();
    void onTrimAccept();
    void onTrimDeny();
    void onGraphMousePress(QMouseEvent *event, QCustomPlot *plot);
    void onGraphMouseMove(QMouseEvent *event, QCustomPlot *plot);
    void onGraphMouseRelease(QMouseEvent *event, QCustomPlot *plot);
    void addTrimLine(double sec, bool isStart);
    void updateTrimLine(double sec, bool isStart);
    void clearTrimSeparators();
    void resetTrim();
    void setTrimInteractionEnabled(bool on);
    double clampToData(double sec) const;

    QWidget *leftWidget;
    QWidget *rightWidget;

    QSplitter *mainSplitter;

    QHBoxLayout *mainLayout;
    QVBoxLayout *leftLayout;
    QVBoxLayout *rigthLayout;

    QGridLayout *rightUnderGraphLayout;

// =================== LEFT SIDE

    QVBoxLayout *settingsLayoutLeft;
    QVBoxLayout *settingsLayoutPort;

    QHBoxLayout *portSettingsLayout;
    QHBoxLayout *opCloseLayout;

    QComboBox *portList;
    QComboBox *baudRate;

    QPushButton *btnConnectPort;
    QPushButton *btnClosePort;
    QPushButton *updatePortList;

    QComboBox *targetDbCombo;

// =================== RECORDED DATA

    QGroupBox   *recordedDataGroup;
    QStackedWidget *recordedDataStack;

    QLabel      *noneModeLabel;

    QLabel      *onceStatusLabel;
    QPushButton *btnSaveOnce;
    QPushButton *btnDiscardOnce;

    QPushButton *btnPrevGesture;
    QPushButton *btnNextGesture;
    QLabel      *listIndexLabel;

// =================== INFO BLOCK

    QGroupBox *infoGroup;
    QLabel    *infoLengthLabel;
    QLabel    *infoFreqLabel;
    QLabel    *infoSamplesLabel;

// =================== TRIM BLOCK

    QGroupBox *trimBlock;
    QPushButton *btnTrimStart;
    QPushButton *btnTrimAccept;
    QPushButton *btnTrimDeny;
    QLabel *recordTimeTrim;

    double m_trimStartSec = -1;
    double m_trimEndSec = -1;
    QVector<QCPItemStraightLine *> m_trimLinesStart;
    QVector<QCPItemStraightLine *> m_trimLinesEnd;
    TrimState m_trimState = TrimState::Off;
    bool m_trimDragging = false;
    bool m_trimDragIsStart = false;
    bool m_rangeDragAccel = false;
    bool m_rangeDragGyro = false;
    static constexpr int TrimDragThresholdPx = 6;

// =================== RIGHT SIDE

    QHBoxLayout *settingTypeMethod;
    QHBoxLayout *settingDBName;
    QHBoxLayout *settingSelectMovement;

    QHBoxLayout *underGraphLayout;

    QVBoxLayout *settingsRightLeftArea;
    QVBoxLayout *settingsRightRightArea;

    QCustomPlot *gyroGraph;
    QCustomPlot *accelGraph;
    QCustomPlot *liveDataPlot;

    QComboBox *methodOfrecord;
    QComboBox *motionType;

    double tempTime = 0;
    double deltaTime = 0;

    bool lastRecordingState = false;

    int snaphots = 0;
    double timeStartSnaphots = 0;

     QList<QCPItemStraightLine*> m_separatorLines;

     QVector<MotionSample> motionSaved;

     WorkMode workMode = WorkMode::WORK_NONE;

signals:
    void updatePortsClicked();
    void connectPortClicked();
    void closePortClicked();

    void targetDatabaseChanged(const QString &dbName);
    void targetMethodChanged(const QString &MethodName);
    void targetMotionTypeChanged(const MotionType &type);

    void saveOnceRequested();
    void discardOnceRequested();
    void prevGestureRequested();
    void nextGestureRequested();
    void trimRequested(double loSec, double hiSec);

    void logMessage(LogLevel level, const QString &text);
};
