#include "core/motionrecorder.h"

#include "core/databasemanager.h"
#include "models/motionsample.h"
#include "models/MotionType.h"
#include <QDebug>
#include <cstring>
#include <cstdint>

MotionRecorder::MotionRecorder(DatabaseManager *dbManager, QObject *parent)
    : QObject(parent)
    , m_dbManager(dbManager)
    , m_targetMethod(QString("None"))
{
}

void MotionRecorder::setTargetDatabase(const QString &dbName) {
    m_targetDb = dbName;
}

void MotionRecorder::setMethodSaves(const QString &methodName) {
    m_targetMethod = methodName;
}

void MotionRecorder::setMotionType(const MotionType &type) {
    m_targetMotionType = type;
}

void MotionRecorder::clearBuffer() {
    m_buffer.clear();
    m_lastRecordState = false;
}

void MotionRecorder::trimBuffer(double loSec, double hiSec) {
    QVector<MotionSample> trimmed;
    for (const MotionSample &s : m_buffer) {
        double t = s.time / 1000000.0;
        if (t >= loSec && t <= hiSec) {
            trimmed.append(s);
        }
    }
    m_buffer = trimmed;

    m_endData.count = static_cast<uint32_t>(m_buffer.size());
    m_endData.crc16 = computeCrc16(m_buffer);
}

void MotionRecorder::onSample(const MotionPacket &MotionPacket) {

    if (m_targetMethod == "None") {
        return;
    }

    if (MotionPacket.recording && !m_lastRecordState) {
        m_buffer.clear();
        m_lastRecordState = true;
        logMessage(LogLevel::Debug, "Recording started");
    }

    if (MotionPacket.recording) {
        m_buffer.append(MotionPacket.sample);
    }

    if (!MotionPacket.recording && m_lastRecordState) {
        m_lastRecordState = false;
        logMessage(LogLevel::Debug, "Recording samples ended, waiting for segment end");
    }
}

void MotionRecorder::insertGesture() {
    if ( m_buffer.size() != m_endData.count) {
        emit logMessage(LogLevel::Warning, QString("The number of saved packets does not match the number of sent packets."));
    }

    if (m_dbManager) {
        m_dbManager->insertGesture(m_targetDb,
                                   m_targetMotionType,
                                   m_buffer,
                                   m_endData.crc16);
    }
}

void MotionRecorder::onSegmentEnd(const SegmentEndPacket &packet) {
    m_endData = packet;
    m_lastRecordState = false;

    logMessage(LogLevel::Debug, QString("Received end packet. Segment count: %1, crc: 0x%2")
               .arg(packet.count).arg(packet.crc16, 0, 16));

    if (m_buffer.isEmpty()) {
        logMessage(LogLevel::Warning, "Segment end received but buffer is empty");
        return;
    }

    if (static_cast<uint32_t>(m_buffer.size()) != packet.count) {
        logMessage(LogLevel::Warning,
                   QString("Sample count mismatch: expected %1, received %2")
                   .arg(packet.count).arg(m_buffer.size()));
    }

    uint16_t computedCrc = computeCrc16(m_buffer);
    if (computedCrc != packet.crc16) {
        logMessage(LogLevel::Warning,
                   QString("CRC mismatch: received 0x%1, computed 0x%2")
                   .arg(packet.crc16, 0, 16).arg(computedCrc, 0, 16));
    }

    emit sampleIsReady(m_buffer);
}

void MotionRecorder::requestSaveGesture() {

    if (m_lastRecordState) {
        logMessage(LogLevel::Error, QString("Can't save data. Recording is in progress."));
        return;
    }

    if (m_buffer.isEmpty()) {
        logMessage(LogLevel::Warning, QString("Can't save: buffer is empty."));
        return;
    }

    insertGesture();

    m_buffer.clear();
}

uint16_t MotionRecorder::computeCrc16(const QVector<MotionSample> &buffer) {
    uint16_t crc = 0xFFFF;
    for (const MotionSample &s : buffer) {
        uint8_t raw[sizeof(MotionSample)];
        std::memcpy(raw, &s, sizeof(MotionSample));

        for (size_t b = 0; b < sizeof(MotionSample); b++) {
            crc ^= static_cast<uint16_t>(raw[b]) << 8;
            for (int i = 0; i < 8; i++) {
                if (crc & 0x8000)
                    crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
                else
                    crc = static_cast<uint16_t>(crc << 1);
            }
        }
    }
    return crc;
}
