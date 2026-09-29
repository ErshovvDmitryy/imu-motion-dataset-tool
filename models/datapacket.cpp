#include "models/datapacket.h"

#include <QStringList>
#include <cmath>

namespace {

// Эвристика для схем, где единица времени не задана: чем крупнее значение,
// тем мельче единица. Точность здесь не критична - встроенная схема IMU
// задаёт микросекунды явно, а для пользовательских схем единицу стоит
// выбрать в редакторе сообщений.
double guessTimeFactor(double value) {
    const double magnitude = std::fabs(value);
    if (magnitude >= 1e12) return 1e-9;  // наносекунды
    if (magnitude >= 1e9)  return 1e-6;  // микросекунды
    if (magnitude >= 1e6)  return 1e-3;  // миллисекунды
    return 1.0;                          // секунды
}

} // namespace

DataFrame::DataFrame()
{
}

DataFrame::DataFrame(const QSharedPointer<const DataSchema> &schema)
    : m_schema(schema)
{
}

DataFrame DataFrame::decode(const QSharedPointer<const DataSchema> &schema,
                           const QByteArray &payload,
                           qint64 hostTimeMs,
                           QString *error) {
    DataFrame frame(schema);
    frame.setHostTimeMs(hostTimeMs);
    frame.setRaw(payload);

    if (schema.isNull()) {
        if (error) {
            *error = QStringLiteral("No schema for the frame");
        }
        return frame;
    }

    if (schema->decode(payload, &frame.m_values, error) == false) {
        return frame;
    }
    return frame;
}

QSharedPointer<const DataSchema> DataFrame::schema() const {
    return m_schema;
}

void DataFrame::setSchema(const QSharedPointer<const DataSchema> &schema) {
    m_schema = schema;
}

const QVector<DataValue> &DataFrame::values() const {
    return m_values;
}

void DataFrame::setValues(const QVector<DataValue> &values) {
    m_values = values;
}

QByteArray DataFrame::raw() const {
    return m_raw;
}

void DataFrame::setRaw(const QByteArray &raw) {
    m_raw = raw;
}

qint64 DataFrame::hostTimeMs() const {
    return m_hostTimeMs;
}

void DataFrame::setHostTimeMs(qint64 ms) {
    m_hostTimeMs = ms;
}

QString DataFrame::schemaName() const {
    return m_schema.isNull() ? QString() : m_schema->name;
}

quint8 DataFrame::typeId() const {
    return m_schema.isNull() ? 0 : m_schema->typeId;
}

bool DataFrame::isValid() const {
    return m_schema.isNull() == false && m_values.isEmpty() == false;
}

bool DataFrame::isEmpty() const {
    return m_values.isEmpty();
}

int DataFrame::count() const {
    return m_values.size();
}

const DataValue *DataFrame::value(const QString &slotName) const {
    if (m_schema.isNull()) {
        return nullptr;
    }
    const int index = m_schema->slotIndex(slotName);
    if (index < 0 || index >= m_values.size()) {
        return nullptr;
    }
    return &m_values.at(index);
}

double DataFrame::toDouble(const QString &slotName, bool *ok) const {
    if (ok) {
        *ok = false;
    }
    const DataValue *found = value(slotName);
    if (found == nullptr) {
        return 0.0;
    }
    return found->toDouble(ok);
}

qint64 DataFrame::toInteger(const QString &slotName, bool *ok) const {
    const DataValue *found = value(slotName);
    if (found == nullptr) {
        if (ok) {
            *ok = false;
        }
        return 0;
    }
    return found->toInt(ok);
}

bool DataFrame::toBool(const QString &slotName, bool *ok) const {
    if (ok) {
        *ok = false;
    }
    const DataValue *found = value(slotName);
    if (found == nullptr) {
        return false;
    }
    if (ok) {
        *ok = true;
    }
    return found->toBool();
}

QString DataFrame::toText(const QString &slotName) const {
    const DataValue *found = value(slotName);
    return found == nullptr ? QString() : found->toText();
}

bool DataFrame::hasTimeField() const {
    return m_schema.isNull() == false && m_schema->roleSlotIndex(FieldRole::Timestamp) >= 0;
}

double DataFrame::timeSeconds(bool *ok) const {
    if (ok) {
        *ok = false;
    }
    if (m_schema.isNull()) {
        return 0.0;
    }

    const int index = m_schema->roleSlotIndex(FieldRole::Timestamp);
    if (index < 0 || index >= m_values.size()) {
        return 0.0;
    }

    const DataValue &raw = m_values.at(index);
    if (raw.isNumeric() == false) {
        return 0.0;
    }

    double factor = 0.0;
    const int fieldIndex = m_schema->slotFieldIndex(index);
    if (fieldIndex >= 0) {
        factor = fieldUnitToSeconds(m_schema->fields.at(fieldIndex).unit);
    }
    if (factor == 0.0) {
        factor = guessTimeFactor(raw.toDouble());
    }

    if (ok) {
        *ok = true;
    }
    return raw.toDouble() * factor;
}

bool DataFrame::hasRecordingField() const {
    return m_schema.isNull() == false && m_schema->roleSlotIndex(FieldRole::Recording) >= 0;
}

bool DataFrame::isRecording(bool *ok) const {
    if (ok) {
        *ok = false;
    }
    if (m_schema.isNull()) {
        return false;
    }
    const int index = m_schema->roleSlotIndex(FieldRole::Recording);
    if (index < 0 || index >= m_values.size()) {
        return false;
    }
    if (ok) {
        *ok = true;
    }
    return m_values.at(index).toBool();
}

bool DataFrame::hasCrcField() const {
    return m_schema.isNull() == false && m_schema->roleSlotIndex(FieldRole::Crc16) >= 0;
}

quint16 DataFrame::crc16(bool *ok) const {
    if (ok) {
        *ok = false;
    }
    if (m_schema.isNull()) {
        return 0;
    }
    const int index = m_schema->roleSlotIndex(FieldRole::Crc16);
    if (index < 0 || index >= m_values.size()) {
        return 0;
    }
    if (ok) {
        *ok = true;
    }
    return static_cast<quint16>(m_values.at(index).toInt());
}

QByteArray DataFrame::crcCoveredBytes() const {
    if (m_schema.isNull()) {
        return QByteArray();
    }
    int covered = 0;
    for (const DataField &field : m_schema->fields) {
        if (field.role == FieldRole::Crc16) {
            break;
        }
        if (field.isVariable()) {
            return m_raw;
        }
        covered += field.byteSize();
    }
    return m_raw.left(covered);
}

QString DataFrame::toString() const {
    if (m_schema.isNull()) {
        return QStringLiteral("<no schema>");
    }

    QStringList parts;
    const QVector<DataValue> &fieldValues = m_values;
    for (int i = 0; i < fieldValues.size(); ++i) {
        const DataValue &item = fieldValues.at(i);
        parts << QStringLiteral("%1=%2").arg(m_schema->slotName(i), item.toText());
    }
    return QStringLiteral("%1{ %2 }").arg(m_schema->name, parts.join(QStringLiteral(", ")));
}
