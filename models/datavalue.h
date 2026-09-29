#pragma once

#include <QByteArray>
#include <QList>
#include <QMetaType>
#include <QString>
#include <QVariant>

// Единственное место в проекте, которое знает про "сырые" типы полей сообщений.
// Обмен с устройством всегда little-endian, независимо от хоста.

enum class DataValueType : int {
    Invalid = 0,
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Float32,
    Float64,
    Bool,
    String,   // фиксированная длина, без терминатора
    Bytes     // фиксированная длина, без интерпретации
};

// Размер одного элемента в байтах. Для String/Bytes 0: длина задаётся в DataField.
int dataValueTypeSize(DataValueType type);

// true для типов с фиксированным размером.
bool dataValueTypeIsFixed(DataValueType type);

// true для типов, которые имеет смысл рисовать на графике.
bool dataValueTypeIsNumeric(DataValueType type);

QString dataValueTypeName(DataValueType type);
DataValueType dataValueTypeFromName(const QString &name);

// Список типов, доступных в редакторе сообщений (без Invalid).
QList<DataValueType> allDataValueTypes();

// Значение одного поля сообщения: тип + QVariant.
class DataValue
{
public:
    DataValue();
    DataValue(DataValueType type, const QVariant &value);
    explicit DataValue(const QVariant &value, DataValueType type = DataValueType::Invalid);

    void clear();
    bool isValid() const;

    DataValueType type() const;
    void setType(DataValueType type);

    const QVariant &raw() const;
    void setRaw(const QVariant &value);

    // Численное приведение. ok == nullptr — не проверять.
    double toDouble(bool *ok = nullptr) const;
    qint64 toInt(bool *ok = nullptr) const;
    bool toBool() const;

    // Отображение: для String возвращает саму строку, иначе число.
    QString toText() const;
    QByteArray toBytes() const;

    bool isNumeric() const;

    // Little-endian десериализация ровно size байт.
    static DataValue fromRaw(DataValueType type, const char *data, int size);
    // Обратная операция. Для строк/байтов используется length.
    QByteArray toRaw(int length = 0) const;

    bool operator==(const DataValue &other) const;
    bool operator!=(const DataValue &other) const { return !(*this == other); }

private:
    DataValueType m_type;
    QVariant m_value;
};

Q_DECLARE_METATYPE(DataValue)
