#include "models/imuadapter.h"

#include "models/builtinschemas.h"

namespace ImuAdapter {

quint16 crc16Ccitt(const QByteArray &data) {
    quint16 crc = 0xFFFF;
    for (char raw : data) {
        crc ^= static_cast<quint16>(static_cast<quint8>(raw)) << 8;
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000) {
                crc = static_cast<quint16>((crc << 1) ^ 0x1021);
            } else {
                crc = static_cast<quint16>(crc << 1);
            }
        }
    }
    return crc;
}

int imuCrcCoveredSize() {
    return 6 * static_cast<int>(sizeof(float)) + static_cast<int>(sizeof(quint32));
}

bool isImuFrame(const DataFrame &frame) {
    return frame.schemaName() == QLatin1String(BuiltinSchemas::imuName);
}

bool dataFrameToMotionSample(const DataFrame &frame, MotionSample *sample) {
    if (sample == nullptr || isImuFrame(frame) == false) {
        return false;
    }

    bool ok = false;

    sample->ax = static_cast<float>(frame.toDouble(QLatin1String(BuiltinSchemas::ImuField::ax), &ok));
    if (ok == false) return false;
    sample->ay = static_cast<float>(frame.toDouble(QLatin1String(BuiltinSchemas::ImuField::ay), &ok));
    if (ok == false) return false;
    sample->az = static_cast<float>(frame.toDouble(QLatin1String(BuiltinSchemas::ImuField::az), &ok));
    if (ok == false) return false;
    sample->gx = static_cast<float>(frame.toDouble(QLatin1String(BuiltinSchemas::ImuField::gx), &ok));
    if (ok == false) return false;
    sample->gy = static_cast<float>(frame.toDouble(QLatin1String(BuiltinSchemas::ImuField::gy), &ok));
    if (ok == false) return false;
    sample->gz = static_cast<float>(frame.toDouble(QLatin1String(BuiltinSchemas::ImuField::gz), &ok));
    if (ok == false) return false;

    return dataFrameToTimestampUs(frame, &sample->time);
}

bool dataFrameToTimestampUs(const DataFrame &frame, quint32 *timestampUs) {
    if (timestampUs == nullptr) {
        return false;
    }
    bool ok = false;
    const double value = frame.toDouble(QLatin1String(BuiltinSchemas::ImuField::time), &ok);
    if (ok == false) {
        return false;
    }
    *timestampUs = static_cast<quint32>(value);
    return true;
}

bool dataFrameToRecording(const DataFrame &frame, bool *recording) {
    if (recording == nullptr) {
        return false;
    }
    bool ok = false;
    const bool value = frame.toBool(QLatin1String(BuiltinSchemas::ImuField::recording), &ok);
    if (ok == false) {
        return false;
    }
    *recording = value;
    return true;
}

bool dataFrameToSegmentCount(const DataFrame &frame, quint16 *count) {
    if (count == nullptr) {
        return false;
    }
    bool ok = false;
    const double value = frame.toDouble(QLatin1String(BuiltinSchemas::SegmentField::count), &ok);
    if (ok == false) {
        return false;
    }
    *count = static_cast<quint16>(value);
    return true;
}

bool dataFrameToSegmentCrc(const DataFrame &frame, quint16 *crc) {
    if (crc == nullptr) {
        return false;
    }
    bool ok = false;
    const double value = frame.toDouble(QLatin1String(BuiltinSchemas::SegmentField::crc16), &ok);
    if (ok == false) {
        return false;
    }
    *crc = static_cast<quint16>(value);
    return true;
}

QSharedPointer<const DataSchema> imuSchemaPtr() {
    static const QSharedPointer<const DataSchema> schema(new DataSchema(BuiltinSchemas::imuLiveMotion()));
    return schema;
}

DataFrame makeImuFrame(float ax, float ay, float az, float gx, float gy, float gz,
                       quint32 timeUs, bool recording) {
    const QSharedPointer<const DataSchema> schema = imuSchemaPtr();
    const DataSchema &definition = *schema;

    QVector<DataValue> values;
    values.reserve(definition.slotCount());
    for (float value : { ax, ay, az, gx, gy, gz }) {
        values.append(DataValue(DataValueType::Float32, QVariant::fromValue(value)));
    }
    values.append(DataValue(DataValueType::UInt32, QVariant::fromValue(timeUs)));
    values.append(DataValue(DataValueType::Bool, QVariant::fromValue(recording)));

    // Полезная часть собирается тем же кодом, что и при разборе потока,
    // поэтому raw всегда соответствует схеме.
    QByteArray payload;
    for (const DataField &field : definition.fields) {
        const DataValueType type = field.type;
        for (int slot = 0; slot < field.count; ++slot) {
            const int index = definition.slotIndex(field.slotName(slot));
            if (index < 0 || index >= values.size()) {
                continue;
            }
            payload.append(values.at(index).toRaw(dataValueTypeSize(type)));
        }
    }

    DataFrame frame = DataFrame::decode(schema, payload);
    if (frame.isValid() == false) {
        // Схема и payload разошлись - не молчим, а отдаём пустой кадр
        // с той же схемой, чтобы вызывающий код не разыменовывал nullptr.
        frame = DataFrame(schema);
    }
    return frame;
}

QVector<DataFrame> motionSamplesToFrames(const QVector<MotionSample> &samples) {
    QVector<DataFrame> frames;
    frames.reserve(samples.size());
    for (const MotionSample &sample : samples) {
        frames.append(makeImuFrame(sample.ax, sample.ay, sample.az,
                                   sample.gx, sample.gy, sample.gz,
                                   sample.time, true));
    }
    return frames;
}

} // namespace ImuAdapter
