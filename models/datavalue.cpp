#include "models/datavalue.h"

#include <QSysInfo>
#include <cstring>

namespace {

bool hostIsLittleEndian() {
    static const bool le = (QSysInfo::ByteOrder == QSysInfo::LittleEndian);
    return le;
}

// Значение собирается в целое без знаковых сдвигов, поэтому int8/int16
// читаются корректно. На хосте с обратным порядком байт результат
// переворачивается, чтобы представление в памяти совпадало с порядком хоста.
quint64 assembleHostOrder(quint64 littleEndianValue, int size) {
    if (hostIsLittleEndian()) {
        return littleEndianValue;
    }
    quint64 swapped = 0;
    for (int i = 0; i < size; ++i) {
        swapped |= ((littleEndianValue >> (8 * i)) & 0xFFULL) << (8 * (size - 1 - i));
    }
    return swapped;
}

quint64 readRawLittleEndian(const char *data, int size) {
    quint64 raw = 0;
    const unsigned char *bytes = reinterpret_cast<const unsigned char *>(data);
    for (int i = 0; i < size; ++i) {
        raw |= static_cast<quint64>(bytes[i]) << (8 * i);
    }
    return raw;
}

template <typename T>
T readLittleEndian(const char *data) {
    const int size = static_cast<int>(sizeof(T));
    const quint64 host = assembleHostOrder(readRawLittleEndian(data, size), size);
    T result = T();
    std::memcpy(&result, &host, sizeof(T));
    return result;
}

template <typename T>
void appendLittleEndian(QByteArray &out, T value) {
    const int size = static_cast<int>(sizeof(T));
    quint64 host = 0;
    std::memcpy(&host, &value, sizeof(T));
    const quint64 little = hostIsLittleEndian() ? host : assembleHostOrder(host, size);
    for (int i = 0; i < size; ++i) {
        out.append(static_cast<char>((little >> (8 * i)) & 0xFFULL));
    }
}

} // namespace

int dataValueTypeSize(DataValueType type) {
    switch (type) {
    case DataValueType::Int8:
    case DataValueType::UInt8:
    case DataValueType::Bool:    return 1;
    case DataValueType::Int16:
    case DataValueType::UInt16:  return 2;
    case DataValueType::Int32:
    case DataValueType::UInt32:
    case DataValueType::Float32: return 4;
    case DataValueType::Float64: return 8;
    case DataValueType::String:
    case DataValueType::Bytes:
    case DataValueType::Invalid: return 0;
    }
    return 0;
}

bool dataValueTypeIsFixed(DataValueType type) {
    return dataValueTypeSize(type) > 0;
}

bool dataValueTypeIsNumeric(DataValueType type) {
    switch (type) {
    case DataValueType::Int8:
    case DataValueType::UInt8:
    case DataValueType::Int16:
    case DataValueType::UInt16:
    case DataValueType::Int32:
    case DataValueType::UInt32:
    case DataValueType::Float32:
    case DataValueType::Float64:
    case DataValueType::Bool:    return true;
    case DataValueType::String:
    case DataValueType::Bytes:
    case DataValueType::Invalid: return false;
    }
    return false;
}

QString dataValueTypeName(DataValueType type) {
    switch (type) {
    case DataValueType::Int8:     return QStringLiteral("int8");
    case DataValueType::UInt8:    return QStringLiteral("uint8");
    case DataValueType::Int16:    return QStringLiteral("int16");
    case DataValueType::UInt16:   return QStringLiteral("uint16");
    case DataValueType::Int32:    return QStringLiteral("int32");
    case DataValueType::UInt32:   return QStringLiteral("uint32");
    case DataValueType::Float32:  return QStringLiteral("float32");
    case DataValueType::Float64:  return QStringLiteral("float64");
    case DataValueType::Bool:     return QStringLiteral("bool");
    case DataValueType::String:   return QStringLiteral("string");
    case DataValueType::Bytes:    return QStringLiteral("bytes");
    case DataValueType::Invalid:  break;
    }
    return QStringLiteral("invalid");
}

DataValueType dataValueTypeFromName(const QString &name) {
    const QString key = name.trimmed().toLower();
    for (DataValueType type : allDataValueTypes()) {
        if (dataValueTypeName(type) == key) {
            return type;
        }
    }

    // Короткие синонимы для DSL.
    if (key == QLatin1String("int"))     return DataValueType::Int32;
    if (key == QLatin1String("uint"))    return DataValueType::UInt32;
    if (key == QLatin1String("float"))   return DataValueType::Float32;
    if (key == QLatin1String("double"))  return DataValueType::Float64;
    if (key == QLatin1String("byte"))    return DataValueType::UInt8;
    if (key == QLatin1String("blob"))    return DataValueType::Bytes;
    if (key == QLatin1String("text"))    return DataValueType::String;

    return DataValueType::Invalid;
}

QList<DataValueType> allDataValueTypes() {
    return QList<DataValueType>{
        DataValueType::Int8,
        DataValueType::UInt8,
        DataValueType::Int16,
        DataValueType::UInt16,
        DataValueType::Int32,
        DataValueType::UInt32,
        DataValueType::Float32,
        DataValueType::Float64,
        DataValueType::Bool,
        DataValueType::String,
        DataValueType::Bytes
    };
}

DataValue::DataValue()
    : m_type(DataValueType::Invalid)
{
}

DataValue::DataValue(DataValueType type, const QVariant &value)
    : m_type(type)
    , m_value(value)
{
}

DataValue::DataValue(const QVariant &value, DataValueType type)
    : m_type(type)
    , m_value(value)
{
}

void DataValue::clear() {
    m_type = DataValueType::Invalid;
    m_value = QVariant();
}

bool DataValue::isValid() const {
    return m_type != DataValueType::Invalid && m_value.isValid();
}

DataValueType DataValue::type() const {
    return m_type;
}

void DataValue::setType(DataValueType type) {
    m_type = type;
}

const QVariant &DataValue::raw() const {
    return m_value;
}

void DataValue::setRaw(const QVariant &value) {
    m_value = value;
}

bool DataValue::isNumeric() const {
    return dataValueTypeIsNumeric(m_type);
}

double DataValue::toDouble(bool *ok) const {
    if (ok) {
        *ok = false;
    }
    if (!isValid()) {
        return 0.0;
    }

    bool converted = false;
    double result = 0.0;

    if (m_type == DataValueType::Bool) {
        result = m_value.toBool() ? 1.0 : 0.0;
        converted = true;
    } else if (isNumeric()) {
        result = m_value.toDouble(&converted);
    } else {
        return 0.0;
    }

    if (ok) {
        *ok = converted;
    }
    return converted ? result : 0.0;
}

qint64 DataValue::toInt(bool *ok) const {
    if (ok) {
        *ok = false;
    }
    if (!isValid()) {
        return 0;
    }

    bool converted = false;
    qint64 result = 0;

    if (m_type == DataValueType::Bool) {
        result = m_value.toBool() ? 1 : 0;
        converted = true;
    } else if (isNumeric()) {
        result = m_value.toLongLong(&converted);
    }

    if (ok) {
        *ok = converted;
    }
    return converted ? result : 0;
}

bool DataValue::toBool() const {
    if (!isValid()) {
        return false;
    }
    if (m_type == DataValueType::Bool) {
        return m_value.toBool();
    }
    return m_value.toDouble() != 0.0;
}

QString DataValue::toText() const {
    if (!m_value.isValid()) {
        return QString();
    }
    if (m_type == DataValueType::String) {
        return m_value.toString();
    }
    if (m_type == DataValueType::Bytes) {
        return QString::fromLatin1(m_value.toByteArray());
    }
    if (m_type == DataValueType::Bool) {
        return m_value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }
    return m_value.toString();
}

QByteArray DataValue::toBytes() const {
    if (!m_value.isValid()) {
        return QByteArray();
    }
    if (m_type == DataValueType::String || m_type == DataValueType::Bytes) {
        return m_value.toByteArray();
    }
    return toRaw();
}

DataValue DataValue::fromRaw(DataValueType type, const char *data, int size) {
    if (!data || size < 0) {
        return DataValue();
    }

    const int needed = dataValueTypeSize(type);
    if (needed > 0) {
        if (size < needed) {
            return DataValue();
        }
        switch (type) {
        case DataValueType::Int8:    return DataValue(type, QVariant::fromValue(static_cast<qint8>(readLittleEndian<qint8>(data))));
        case DataValueType::UInt8:
        case DataValueType::Bool:    return DataValue(type, QVariant::fromValue(static_cast<quint8>(readLittleEndian<quint8>(data))));
        case DataValueType::Int16:   return DataValue(type, QVariant::fromValue(static_cast<qint16>(readLittleEndian<qint16>(data))));
        case DataValueType::UInt16:  return DataValue(type, QVariant::fromValue(static_cast<quint16>(readLittleEndian<quint16>(data))));
        case DataValueType::Int32:   return DataValue(type, QVariant::fromValue(static_cast<qint32>(readLittleEndian<qint32>(data))));
        case DataValueType::UInt32:  return DataValue(type, QVariant::fromValue(static_cast<quint32>(readLittleEndian<quint32>(data))));
        case DataValueType::Float32: return DataValue(type, QVariant::fromValue(readLittleEndian<float>(data)));
        case DataValueType::Float64: return DataValue(type, QVariant::fromValue(readLittleEndian<double>(data)));
        default: break;
        }
        return DataValue();
    }

    if (type == DataValueType::String) {
        return DataValue(type, QVariant::fromValue(QString::fromLatin1(data, size)));
    }
    if (type == DataValueType::Bytes) {
        return DataValue(type, QVariant::fromValue(QByteArray(data, size)));
    }
    return DataValue();
}

QByteArray DataValue::toRaw(int length) const {
    QByteArray out;

    switch (m_type) {
    case DataValueType::Int8:
        appendLittleEndian(out, static_cast<qint8>(m_value.toLongLong()));
        break;
    case DataValueType::Int16:
        appendLittleEndian(out, static_cast<qint16>(m_value.toLongLong()));
        break;
    case DataValueType::Int32:
        appendLittleEndian(out, static_cast<qint32>(m_value.toLongLong()));
        break;
    case DataValueType::UInt8:
        appendLittleEndian(out, static_cast<quint8>(m_value.toULongLong()));
        break;
    case DataValueType::UInt16:
        appendLittleEndian(out, static_cast<quint16>(m_value.toULongLong()));
        break;
    case DataValueType::UInt32:
        appendLittleEndian(out, static_cast<quint32>(m_value.toULongLong()));
        break;
    case DataValueType::Bool:
        out.append(m_value.toBool() ? '\x01' : '\x00');
        break;
    case DataValueType::Float32:
        appendLittleEndian(out, m_value.toFloat());
        break;
    case DataValueType::Float64:
        appendLittleEndian(out, m_value.toDouble());
        break;
    case DataValueType::String:
    case DataValueType::Bytes:
        out = m_value.toByteArray();
        break;
    case DataValueType::Invalid:
        break;
    }

    if (length > 0) {
        out.truncate(length);
    }
    return out;
}

bool DataValue::operator==(const DataValue &other) const {
    if (m_type != other.m_type) {
        return false;
    }
    if (!m_value.isValid() || !other.m_value.isValid()) {
        return m_value.isValid() == other.m_value.isValid();
    }
    if (m_type == DataValueType::Float32 || m_type == DataValueType::Float64) {
        return qFuzzyCompare(m_value.toDouble() + 1.0, other.m_value.toDouble() + 1.0);
    }
    return m_value == other.m_value;
}
