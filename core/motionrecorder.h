#pragma once

#include <QObject>
#include <QVector>

#include "models/motionsample.h"
#include "models/MotionType.h"

class DatabaseManager;

class MotionRecorder : public QObject
{
    Q_OBJECT

public:
    explicit MotionRecorder(DatabaseManager *dbManager, QObject *parent = nullptr);

    void setRecording(bool enabled);
    void setTargetDatabase(const QString &dbName);

public slots:
    void onSample(const MotionPacket &packet);
    //void onSegmentEnd(const MotionPacket &packet);

private:
    DatabaseManager *m_dbManager = nullptr;
    bool m_recordState = false;
    bool m_lastRecordState = false;
    QString m_targetDb;
    QVector<MotionSample> m_buffer;
};
