#include "core/motionrecorder.h"

#include "core/databasemanager.h"
#include "models/imuadapter.h"
#include "models/builtinschemas.h"

#include <QStringList>

MotionRecorder::MotionRecorder(QObject *parent)
    : QObject(parent)
    , m_targetMethod(QStringLiteral("None"))
{
    setSchemas(BuiltinSchemas::all());
    m_recordedSchemas = QStringList{ QString::fromLatin1(BuiltinSchemas::imuName) };
}

void MotionRecorder::setDatabaseManager(DatabaseManager *manager) {
    m_dbManager = manager;
}

void MotionRecorder::setSchemas(const QList<DataSchema> &schemas) {
    m_schemas.clear();
    for (const DataSchema &schema : schemas) {
        m_schemas.insert(schema.name, schema);
    }
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

void MotionRecorder::setRecordedSchemas(const QStringList &schemaNames) {
    m_recordedSchemas = schemaNames;
}

void MotionRecorder::clearBuffer() {
    m_buffer.clear();
    m_lastRecordState = false;
    m_startFlag = -1;
    m_endFlag = -1;
    emit bufferChanged(0);
}

DataField MotionRecorder::fieldWithRole(const DataFrame &frame, FieldRole role) const {
    const auto found = m_schemas.constFind(frame.schemaName());
    if (found == m_schemas.constEnd()) {
        return DataField();
    }
    for (const DataField &field : found.value().fields) {
        if (field.role == role) {
            return field;
        }
    }
    return DataField();
}

bool MotionRecorder::isRecorded(const DataFrame &frame) const {
    if (m_recordedSchemas.contains(frame.schemaName()) == false) {
        return false;
    }
    const DataField field = fieldWithRole(frame, FieldRole::Recording);
    if (field.name.isEmpty()) {
        // Нет флага записи - считаем кадр пишущимся всегда.
        return true;
    }
    return frame.toBool(field.name, nullptr);
}

bool MotionRecorder::timestampSeconds(const DataFrame &frame, double *seconds) const {
    const DataField field = fieldWithRole(frame, FieldRole::Timestamp);
    if (field.name.isEmpty()) {
        return false;
    }
    bool ok = false;
    const double value = frame.timeSeconds(&ok);
    if (ok == false) {
        return false;
    }
    *seconds = value;
    return true;
}

bool MotionRecorder::isSegmentEnd(const DataFrame &frame) const {
    return fieldWithRole(frame, FieldRole::Crc16).name.isEmpty() == false;
}

quint16 MotionRecorder::segmentCountOf(const DataFrame &frame, bool *ok) const {
    const DataField crc = fieldWithRole(frame, FieldRole::Crc16);
    if (crc.name.isEmpty()) {
        if (ok) *ok = false;
        return 0;
    }
    for (const DataField &field : m_schemas.value(frame.schemaName()).fields) {
        if (field.name == crc.name) {
            // count - это поле, стоящее непосредственно перед CRC.
            const int index = m_schemas.value(frame.schemaName()).fields.indexOf(field);
            if (index > 0) {
                const DataField count = m_schemas.value(frame.schemaName()).fields.at(index - 1);
                bool valid = false;
                const qint64 value = frame.toInteger(count.name, &valid);
                if (ok) *ok = valid;
                return static_cast<quint16>(value);
            }
        }
    }
    if (ok) *ok = false;
    return 0;
}

quint16 MotionRecorder::segmentCrcOf(const DataFrame &frame, bool *ok) const {
    const DataField crc = fieldWithRole(frame, FieldRole::Crc16);
    if (crc.name.isEmpty()) {
        if (ok) *ok = false;
        return 0;
    }
    bool valid = false;
    const qint64 value = frame.toInteger(crc.name, &valid);
    if (ok) *ok = valid;
    return static_cast<quint16>(value);
}

quint16 MotionRecorder::bufferCrc() const {
    const int covered = ImuAdapter::imuCrcCoveredSize();
    QByteArray raw;
    raw.reserve(m_buffer.size() * covered);
    for (const DataFrame &frame : m_buffer) {
        raw.append(frame.raw().left(covered));
    }
    return ImuAdapter::crc16Ccitt(raw);
}

void MotionRecorder::trimBuffer(double loSec, double hiSec) {
    QVector<DataFrame> trimmed;
    for (const DataFrame &frame : m_buffer) {
        double seconds = 0.0;
        if (timestampSeconds(frame, &seconds) == false) {
            continue;
        }
        if (seconds >= loSec && seconds <= hiSec) {
            trimmed.append(frame);
        }
    }
    m_buffer = trimmed;
    emit bufferChanged(m_buffer.size());
}

void MotionRecorder::setFlags(int startFlag, int endFlag) {
    m_startFlag = startFlag;
    m_endFlag = endFlag;
}

int MotionRecorder::gestureCount() const {
    return m_gestures.size();
}

int MotionRecorder::currentGesture() const {
    return m_currentKey;
}

void MotionRecorder::prevGesture() {
    if (m_gestures.contains(m_currentKey - 1) == false) {
        return;
    }
    --m_currentKey;
    emit selectedSample(m_gestures.value(m_currentKey).frames, m_currentKey);
}

void MotionRecorder::nextGesture() {
    if (m_gestures.contains(m_currentKey + 1) == false) {
        return;
    }
    ++m_currentKey;
    emit selectedSample(m_gestures.value(m_currentKey).frames, m_currentKey);
}

void MotionRecorder::onFrame(const DataFrame &frame) {
    if (m_targetMethod == QStringLiteral("None")) {
        return;
    }
    if (isSegmentEnd(frame)) {
        onSegmentEnd(frame);
        return;
    }

    const bool recording = isRecorded(frame);
    if (recording && m_lastRecordState == false) {
        m_buffer.clear();
        m_lastRecordState = true;
        emit logMessage(LogLevel::Debug, QStringLiteral("Recording started"));
    }

    if (recording) {
        m_buffer.append(frame);
    }

    if (recording == false && m_lastRecordState) {
        m_lastRecordState = false;
        emit logMessage(LogLevel::Debug,
                        QStringLiteral("Recording samples ended, waiting for segment end"));
    }
}

void MotionRecorder::insertGesture() {
    QVector<MotionSample> samples;
    samples.reserve(m_buffer.size());
    for (const DataFrame &frame : m_buffer) {
        MotionSample sample;
        if (ImuAdapter::dataFrameToMotionSample(frame, &sample)) {
            samples.append(sample);
        }
    }

    if (samples.isEmpty()) {
        emit logMessage(LogLevel::Warning,
                        QStringLiteral("Frames cannot be stored: no IMU samples in the buffer"));
        return;
    }

    if (m_dbManager == nullptr) {
        return;
    }

    const int sampleId = m_dbManager->insertGesture(m_targetDb, m_targetMotionType,
                                                    samples, bufferCrc());
    if (sampleId >= 0 && m_startFlag >= 0 && m_endFlag >= 0) {
        m_dbManager->updateFlags(m_targetDb, sampleId, m_startFlag, m_endFlag);
    }
    m_startFlag = -1;
    m_endFlag = -1;
}

void MotionRecorder::onSegmentEnd(const DataFrame &frame) {
    m_lastRecordState = false;

    bool countOk = false;
    const quint16 count = segmentCountOf(frame, &countOk);
    bool crcOk = false;
    const quint16 crc = segmentCrcOf(frame, &crcOk);

    emit logMessage(LogLevel::Debug,
                    QStringLiteral("Received end packet. Segment count: %1, crc: 0x%2")
                        .arg(count)
                        .arg(crc, 0, 16));

    if (m_buffer.isEmpty()) {
        emit logMessage(LogLevel::Warning, QStringLiteral("Segment end received but buffer is empty"));
        return;
    }

    if (countOk && static_cast<quint32>(m_buffer.size()) != count) {
        emit logMessage(LogLevel::Warning,
                        QStringLiteral("Sample count mismatch: expected %1, received %2")
                            .arg(count)
                            .arg(m_buffer.size()));
    }

    if (crcOk) {
        const quint16 computed = bufferCrc();
        if (computed != crc) {
            emit logMessage(LogLevel::Warning,
                            QStringLiteral("CRC mismatch: received 0x%1, computed 0x%2")
                                .arg(crc, 0, 16)
                                .arg(computed, 0, 16));
        }
    }

    if (m_targetMethod == QStringLiteral("List")) {
        Gesture gesture;
        gesture.frames = m_buffer;
        gesture.count = count;
        gesture.crc = crc;
        m_gestures.insert(m_nextKey, gesture);

        m_currentKey = m_nextKey;
        ++m_nextKey;

        emit updateSizeList(m_gestures.size());
        emit sampleIsReady(m_buffer, m_currentKey);
        return;
    }

    emit sampleIsReady(m_buffer, 0);
}

void MotionRecorder::requestSaveGesture() {
    if (m_lastRecordState) {
        emit logMessage(LogLevel::Error, QStringLiteral("Can't save data. Recording is in progress."));
        return;
    }
    if (m_buffer.isEmpty()) {
        emit logMessage(LogLevel::Warning, QStringLiteral("Can't save: buffer is empty."));
        return;
    }

    insertGesture();
    clearBuffer();
}
