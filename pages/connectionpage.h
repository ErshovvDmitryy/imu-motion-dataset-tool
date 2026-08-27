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
class QCheckBox;
class QCustomPlot;
class QCPItemStraightLine;
class QLabel;
class QGroupBox;
class QStackedWidget;

class ConnectionPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectionPage(QWidget *parent = nullptr);

    void drawSeparator();
    void updateStream(const MotionPacket &packet);
    void setPorts(const QStringList &ports);
    void setAvailableDatabases(const QStringList &databases);
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

    QWidget *leftWidget;
    QWidget *rightWidget;

    QSplitter *mainSplitter;

    QHBoxLayout *mainLayout;
    QVBoxLayout *leftLayout;
    QVBoxLayout *rigthLayout;

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

    QCheckBox *recordCheckBox;
    QComboBox *targetDbCombo;

// =================== RECORDED DATA (right, under recordCheckBox)

    QGroupBox   *recordedDataGroup;
    QStackedWidget *recordedDataStack;

    QLabel      *noneModeLabel;

    QLabel      *onceStatusLabel;
    QPushButton *btnSaveOnce;
    QPushButton *btnDiscardOnce;

    QPushButton *btnPrevGesture;
    QPushButton *btnNextGesture;
    QLabel      *listIndexLabel;

// =================== INFO BLOCK (right, under recorded data)

    QGroupBox *infoGroup;
    QLabel    *infoLengthLabel;
    QLabel    *infoFreqLabel;
    QLabel    *infoSamplesLabel;

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

signals:
    void updatePortsClicked();
    void connectPortClicked();
    void closePortClicked();

    void recordingToggled(bool enabled);
    void targetDatabaseChanged(const QString &dbName);

    void saveOnceRequested();
    void discardOnceRequested();
    void prevGestureRequested();
    void nextGestureRequested();

    void logMessage(LogLevel level, const QString &text);
};
