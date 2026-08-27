#include "core/motionrecorder.h"

#include "core/databasemanager.h"
#include "models/motionsample.h"
#include "models/MotionType.h"
#include <QDebug>

MotionRecorder::MotionRecorder(DatabaseManager *dbManager, QObject *parent)
    : QObject(parent)
    , m_dbManager(dbManager)
{
}

void MotionRecorder::setRecording(bool enabled) {
    m_recordState = enabled;
    if (!m_recordState)
        m_buffer.clear();
}

void MotionRecorder::setTargetDatabase(const QString &dbName) {
    m_targetDb = dbName;
}

void MotionRecorder::onSample(const MotionPacket &MotionPacket) {
    if ( m_recordState && MotionPacket.recording && !m_lastRecordState) {
        m_buffer.clear();
        m_lastRecordState = true;
    }

    if ( m_recordState && MotionPacket.recording ) {
        m_buffer.append(MotionPacket.sample);
    }

    if ( m_recordState && !MotionPacket.recording && m_lastRecordState ) {
        if (m_dbManager) {
            m_dbManager->insertGesture(m_targetDb,
                                       MotionType::Unknown,
                                       m_buffer,
                                       124);
        }
        m_lastRecordState = false;
    }

}

/*void MotionRecorder::onSegmentEnd(const MotionPacket &packet) {
    if (packet.recording) {
        m_buffer.append(packet.sample);
    }

    if (m_dbManager) {
        m_dbManager->insertGesture(m_targetDb,
                                   MotionType::Unlabeled,
                                   m_buffer,
                                   packet.crc16);
    }

    qDebug() << "tyt da3213";
    m_buffer.clear();
}*/
