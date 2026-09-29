#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QVector>

#include "models/datapacket.h"
#include "models/dataschema.h"
#include "models/loglevel.h"
#include "models/MotionType.h"

class DatabaseManager;

// Буфер записи жеста. Работает с DataFrame: приём общий для любой схемы,
// а запись в базу выполняется только для встроенной схемы IMU, потому что
// DatabaseManager хранит фиксированную раскладку samples.
class MotionRecorder : public QObject
{
    Q_OBJECT

public:
    explicit MotionRecorder(QObject *parent = nullptr);

    void setDatabaseManager(DatabaseManager *manager);
    // Схема, из которой берётся роль Timestamp и Recording.
    void setSchemas(const QList<DataSchema> &schemas);
    void setTargetDatabase(const QString &dbName);
    void setMethodSaves(const QString &methodName);
    void setMotionType(const MotionType &type);
    // Список имён сообщений, которые пишутся в базу как IMU-жесты.
    void setRecordedSchemas(const QStringList &schemaNames);

    void clearBuffer();
    void trimBuffer(double loSec, double hiSec);
    void setFlags(int startFlag, int endFlag);

    void prevGesture();
    void nextGesture();

    int gestureCount() const;
    int currentGesture() const;

public slots:
    // Единая точка входа: любой декодированный кадр.
    void onFrame(const DataFrame &frame);
    void onSegmentEnd(const DataFrame &frame);
    void requestSaveGesture();

signals:
    void sampleIsReady(const QVector<DataFrame> &frames, int key);
    void updateSizeList(int listSize);
    void selectedSample(const QVector<DataFrame> &frames, int key);
    void bufferChanged(int size);
    void logMessage(LogLevel level, const QString &text);

private:
    struct Gesture {
        QVector<DataFrame> frames;
        quint16 count = 0;
        quint16 crc = 0;
    };

    // Роли ищутся по схеме кадра, а не по имени, чтобы нековенно
    // зависеть от того, что сообщение называется LiveMotion.
    DataField fieldWithRole(const DataFrame &frame, FieldRole role) const;
    bool isRecorded(const DataFrame &frame) const;
    bool timestampSeconds(const DataFrame &frame, double *seconds) const;
    bool isSegmentEnd(const DataFrame &frame) const;
    void insertGesture();
    quint16 bufferCrc() const;
    quint16 segmentCountOf(const DataFrame &frame, bool *ok) const;
    quint16 segmentCrcOf(const DataFrame &frame, bool *ok) const;

    QHash<QString, DataSchema> m_schemas;
    QStringList m_recordedSchemas;
    DatabaseManager *m_dbManager = nullptr;

    bool m_lastRecordState = false;
    QString m_targetDb;
    QString m_targetMethod;
    MotionType m_targetMotionType = MotionType::Unlabeled;

    QVector<DataFrame> m_buffer;

    QMap<int, Gesture> m_gestures;
    int m_nextKey = 1;
    int m_currentKey = 0;

    int m_startFlag = -1;
    int m_endFlag = -1;
};
