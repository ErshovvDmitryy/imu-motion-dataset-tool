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

// =================== Work modes:
// 1. None - live graph in real time, accel and gyro graph draw motion. Can't save motion
// 2. Once - live graph in real time, accel and gyro graph draw motion.
//                              Saved last motion in buffer. User can save and edit motion
// 3. List - live graph in real time, accel and gyro graph draw motion. Motions saved in buffer QVector<Motion>
//                                  User can switch, edit and saved  motion.

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

    void setPorts(const QStringList &ports);
    void setAvailableDatabases(const QStringList &databases);

    void updateStream(const MotionPacket &packet);              // Method for draw real time graph
    void incomingSegment(QVector<MotionSample> &receivedData); // Method for draw saved motion in graph

    void drawSeparator();               // Draw separators in real time graph
    void drawSavedSegmentSeparators();  // Draw separators for saved motion

    PortConfig connectTo();     // Connect to port selected from QComboBox portList

    void setWorkMode(WorkMode &mode);
    WorkMode getWorkMode() const;

private:

    void createWidgets();
    void createLayouts();
    void connectSignals();

// =================== Func for setups widgets

    void setupGraphs();
    void setupComboBox();
    void setupButtonsOnPage();
    void setupRecordedDataWidget();
    void setupInfoBlockWidget();
    void setupTrimBlockWidget();
    void setupDataBaseBlockWidget();

// =================== Func for setups layouts

    void setupPortSettingsLayout();

// =================== Private function: GRAPH func

    void clearAccelGyroGraphs();
    void clearAllGraphs();
    void clearSeparators();

    void updateGyroGraph(const MotionSample &sample);
    void updateAccelGraph(const MotionSample &sample);
    void updateliveDataPlotGraph(const MotionSample &sample);
    void updateSavedGraph(QVector<MotionSample> &motionSaved);

    void addSeparator(QCustomPlot *plot, double x);

    void updateTime(const uint32_t time);
    void resetTime();

    void resetCountSnapshots() { snaphots = 0; }

    void updateInfoBox(const float &infoLength, const float &infoFreqLabel, const int infoSamplesLabel);
    void onSaveOnceClicked();

// =================== Private function: TRIM func

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


// =================== MAIN PAGE LAYOUTS DECLARATION

    QWidget *leftWidget;
    QWidget *rightWidget;

    QSplitter *mainSplitter;

    QHBoxLayout *mainLayout;
    QVBoxLayout *leftLayout;

    QVBoxLayout *rightLayout;
    QGridLayout *rightUnderGraphLayout;

// =================== LEFT SIDE DECLARATION

    QVBoxLayout *settingsLayoutLeft;
    QVBoxLayout *settingsLayoutPort;

    QHBoxLayout *portSettingsLayout;
    QHBoxLayout *portButtonsLayout;

    QComboBox *portList;
    QComboBox *baudRate;

    QPushButton *btnConnectPort;
    QPushButton *btnClosePort;
    QPushButton *updatePortList;

// =================== RECORDED DATA DECLARATION

    QGroupBox *recordedDataGroup;
    QStackedWidget *recordedDataStack;

    QLabel *noneModeLabel;

    QLabel *onceStatusLabel;
    QPushButton *btnSaveOnce;
    QPushButton *btnDiscardOnce;

    QPushButton *btnPrevGesture;
    QPushButton *btnNextGesture;
    QLabel *listIndexLabel;

// =================== INFO BLOCK DECLARATION

    QGroupBox *infoGroup;
    QLabel *infoLengthLabel;
    QLabel *infoFreqLabel;
    QLabel *infoSamplesLabel;

// =================== TRIM BLOCK DECLARATION

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


// =================== DATABASE BLOCK DECLARATION

    QGroupBox *databaseBlock;

    QComboBox *targetDbCombo;
    QComboBox *recordingMethod;
    QComboBox *motionType;

// =================== RIGHT SIDE DECLARATION

    QVBoxLayout *settingsRightLeftArea;
    QVBoxLayout *settingsRightRightArea;

// =================== GRAPH AND RELATED

    QCustomPlot *gyroGraph;
    QCustomPlot *accelGraph;
    QCustomPlot *liveDataPlot;

    double tempTime = 0;
    double deltaTime = 0;

    bool lastRecordingState = false;

    int snaphots = 0;
    double timeStartSnaphots = 0;

     QList<QCPItemStraightLine*> m_separatorLines;

     WorkMode workMode = WorkMode::WORK_NONE;

     QVector<MotionSample> motionSaved;

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
