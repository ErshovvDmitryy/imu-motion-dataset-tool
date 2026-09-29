#include "models/builtinschemas.h"

#include "models/packettype.h"

namespace BuiltinSchemas {

DataSchema imuLiveMotion() {
    DataSchema schema;
    schema.name = QLatin1String(imuName);
    schema.description = QStringLiteral("Built-in IMU live sample (matches the legacy MotionSample layout)");
    schema.typeId = static_cast<quint8>(PacketType::LiveMotion);
    schema.framing = FramingMode::FixedLength;
    schema.builtIn = true;

    const char *axes[] = { ImuField::ax, ImuField::ay, ImuField::az,
                           ImuField::gx, ImuField::gy, ImuField::gz };
    for (const char *axis : axes) {
        DataField field;
        field.name = QLatin1String(axis);
        field.type = DataValueType::Float32;
        schema.fields.append(field);
    }

    DataField time;
    time.name = QLatin1String(ImuField::time);
    time.type = DataValueType::UInt32;
    time.role = FieldRole::Timestamp;
    time.unit = FieldUnit::Microseconds;
    schema.fields.append(time);

    DataField recording;
    recording.name = QLatin1String(ImuField::recording);
    recording.type = DataValueType::Bool;
    recording.role = FieldRole::Recording;
    schema.fields.append(recording);

    return schema;
}

DataSchema imuSegmentEnd() {
    DataSchema schema;
    schema.name = QLatin1String(segmentName);
    schema.description = QStringLiteral("Built-in end-of-segment marker: sample count and segment CRC-16");
    schema.typeId = static_cast<quint8>(PacketType::SegmentEnd);
    schema.framing = FramingMode::FixedLength;
    schema.builtIn = true;

    DataField count;
    count.name = QLatin1String(SegmentField::count);
    count.type = DataValueType::UInt16;
    schema.fields.append(count);

    // Роль Crc16 здесь намеренно не ставится: это сумма всего сегмента,
    // а не контрольная сумма собственного кадра. Проверку делает
    // MotionRecorder, складывая байты записанных сообщений.
    DataField crc;
    crc.name = QLatin1String(SegmentField::crc16);
    crc.type = DataValueType::UInt16;
    schema.fields.append(crc);

    return schema;
}

QList<DataSchema> all() {
    return QList<DataSchema>{ imuLiveMotion(), imuSegmentEnd() };
}

} // namespace BuiltinSchemas
