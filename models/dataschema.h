#pragma once

#include <QJsonObject>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVector>

#include "models/datavalue.h"
// Как определить границы кадра в потоке байт.
enum class FramingMode : int {
    // [typeId:u8][payload] - длина берётся из схемы. Совпадает с текущим
    // протоколом ESP32, поэтому встроенная схема IMU работает без правок.
    FixedLength = 0,
    // [typeId:u8][len:u16][payload] - длина приходит в кадре, поля с
    // переменной длиной (string/bytes без length) занимают остаток.
    HeaderLength
};

// Назначение поля. Определяет, как ядро работает с сообщением.
enum class FieldRole : int {
    None = 0,
    Timestamp,  // значение времени, по нему строится ось X
    Crc16,      // контрольная сумма CRC-16/CCITT-FALSE, обязана быть последней
    Recording   // bool: устройство сейчас пишет поток
};

// Единица значения времени. Значение 0 означает «подобрать автоматически».
enum class FieldUnit : int {
    None = 0,
    Seconds,
    Milliseconds,
    Microseconds,
    Nanoseconds
};

QString fieldRoleName(FieldRole role);
FieldRole fieldRoleFromName(const QString &name);
QString fieldUnitName(FieldUnit unit);
FieldUnit fieldUnitFromName(const QString &name);

// Множитель для перевода значения в секунды.
double fieldUnitToSeconds(FieldUnit unit);

// Одно поле сообщения. count > 1 — массив значений подряд в кадре.
struct DataField {
    QString name;
    DataValueType type = DataValueType::Int32;
    int count = 1;      // сколько раз значение повторяется подряд
    int length = 0;     // байтовая длина для string/bytes
    FieldRole role = FieldRole::None;
    FieldUnit unit = FieldUnit::None;

    bool isArray() const { return count > 1; }
    bool isVariable() const;      // string/bytes без заданной длины
    int elementSize() const;      // байт на одно значение
    int byteSize() const;         // байт на всё поле
    int slotCount() const;        // сколько значений попадёт в DataFrame
    bool isPlotable() const;      // есть смысл рисовать

    // "values" или "values[0..9]"
    QString label() const;
    // Имя одного развёрнутого значения: "values" или "values[3]"
    QString slotName(int slot) const;

    bool operator==(const DataField &other) const;
    bool operator!=(const DataField &other) const { return !(*this == other); }
};

// Развёрнутое поле, пригодное для выбора переменных на графике.
struct PlottedField {
    QString name;                 // "values[3]"
    int fieldIndex = 0;
    int slot = 0;
    DataValueType type = DataValueType::Invalid;
    bool numeric = false;
};

Q_DECLARE_METATYPE(PlottedField)

// Описание типа входящего сообщения: имя, код в потоке и список полей.
class DataSchema
{
public:
    DataSchema();

    QString name;
    QString description;
    quint8 typeId = 0;
    FramingMode framing = FramingMode::FixedLength;
    bool builtIn = false;
    QVector<DataField> fields;

    bool isValid() const;
    // Человекочитаемая причина, почему isValid() == false.
    QString validationError() const;

    int payloadSize() const;    // точный размер полезной части
    int minPayloadSize() const; // размер, если переменные поля пустые
    int headerSize() const;     // 1 или 3 байта
    int totalSize() const;
    int slotCount() const;      // сколько значений в DataFrame
    bool hasVariableFields() const;

    int fieldIndex(const QString &fieldName) const;
    // Индекс поля, которому принадлежит развёрнутое значение.
    int slotFieldIndex(int slot) const;
    // Индекс значения в развёрнутом векторе DataFrame::values().
    int slotIndex(const QString &slotName) const;
    QStringList slotNames() const;
    // Имя развёрнутого значения по его индексу.
    QString slotName(int slot) const;
    int roleSlotIndex(FieldRole role) const;

    QVector<PlottedField> plottableFields() const;
    QStringList plottableSlotNames() const;
    // Имя сюжета для легенды: имя поля + отображаемая единица.
    QString traceLabel(const PlottedField &field) const;
    // То же самое, но по развёрнутому имени значения.
    QString slotLabel(const QString &slotName) const;

    // Разобрать полезную часть кадра в развёрнутый вектор значений.
    // Роли Timestamp/Crc16/Recording ни на что не влияют - это метаданные схемы.
    bool decode(const QByteArray &payload, QVector<DataValue> *out, QString *error = nullptr) const;

    QJsonObject toJson() const;
    static DataSchema fromJson(const QJsonObject &object, QString *error = nullptr);

    // DSL: "ax:float32; values:int16*10; t:uint32@time@us; crc:uint16@crc"
    // Разделитель полей - точка с запятой или перевод строки.
    static QVector<DataField> parseFieldList(const QString &text, QString *error = nullptr);
    static QString fieldListToDsl(const QVector<DataField> &fields);
    QString dsl() const;

    static QString framingName(FramingMode mode);
    static FramingMode framingFromName(const QString &name);

    bool operator==(const DataSchema &other) const;
    bool operator!=(const DataSchema &other) const { return !(*this == other); }
};

Q_DECLARE_METATYPE(DataSchema)
