#pragma once

#include <QObject>
#include <QVector>
#include <QMap>

#include "models/motionsample.h"
#include "models/MotionType.h"
#include "models/loglevel.h"
#include "widgets/consolewidget.h"

class DatabaseManager;

class MotionRecorder : public QObject
{
    Q_OBJECT

public:
    explicit MotionRecorder(DatabaseManager *dbManager, QObject *parent = nullptr);

    void setTargetDatabase(const QString &dbName);
    void setMethodSaves(const QString &methodName);
    void setMotionType(const MotionType &type);
    void clearBuffer();
    void trimBuffer(double loSec, double hiSec);
    void setFlags(int startFlag, int endFlag);

public slots:
    void onSample(const MotionPacket &packet);
    void onSegmentEnd(const SegmentEndPacket &packet);

    void requestSaveGesture();


private:
    void insertGesture();

    DatabaseManager *m_dbManager = nullptr;
    bool m_lastRecordState = false;
    QString m_targetDb;
    QString m_targetMethod;
    MotionType m_targetMotionType;

    SegmentEndPacket m_endData;
    QVector<MotionSample> m_buffer;

    int m_startFlag = -1;
    int m_endFlag = -1;


    QMap<int, QVector<MotionSample>> m_mapBuffer;

    static uint16_t computeCrc16(const QVector<MotionSample> &buffer);

signals:

    void sampleIsReady(QVector<MotionSample> &receivedData);
    void logMessage(LogLevel level, const QString &text);

};
