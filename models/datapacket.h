#pragma once

#include <QByteArray>
#include <QMetaType>
#include <QSharedPointer>
#include <QString>
#include <QVector>

#include "models/dataschema.h"
#include "models/datavalue.h"

// Один экземпляр входящего сообщения: значения, разобранные по схеме.
// Схема хранится разделяемым указателем, поэтому копия кадра не тянет
// за собой копию описания сообщения.
class DataFrame
{
public:
    DataFrame();
    explicit DataFrame(const QSharedPointer<const DataSchema> &schema);

    // Разобрать полезную часть кадра согласно схеме.
    static DataFrame decode(const QSharedPointer<const DataSchema> &schema,
                            const QByteArray &payload,
                            qint64 hostTimeMs = 0,
                            QString *error = nullptr);

    QSharedPointer<const DataSchema> schema() const;
    void setSchema(const QSharedPointer<const DataSchema> &schema);

    const QVector<DataValue> &values() const;
    void setValues(const QVector<DataValue> &values);

    QByteArray raw() const;
    void setRaw(const QByteArray &raw);

    qint64 hostTimeMs() const;
    void setHostTimeMs(qint64 ms);

    QString schemaName() const;
    quint8 typeId() const;
    bool isValid() const;
    bool isEmpty() const;
    int count() const;

    // Доступ по развёрнутому имени: "ax" или "values[3]".
    const DataValue *value(const QString &slotName) const;
    double toDouble(const QString &slotName, bool *ok = nullptr) const;
    // Целочисленные поля (count, crc16, номера) удобнее читать так.
    qint64 toInteger(const QString &slotName, bool *ok = nullptr) const;
    bool toBool(const QString &slotName, bool *ok = nullptr) const;
    QString toText(const QString &slotName) const;

    // Время в секундах по полю с ролью Timestamp.
    double timeSeconds(bool *ok = nullptr) const;
    bool hasTimeField() const;

    // Признак записи: значение поля с ролью Recording.
    bool isRecording(bool *ok = nullptr) const;
    bool hasRecordingField() const;

    // Значение поля с ролью Crc16 (0, если поля нет).
    quint16 crc16(bool *ok = nullptr) const;
    bool hasCrcField() const;
    // Байты полезной части, попадающие под контрольную сумму.
    QByteArray crcCoveredBytes() const;

    // "LiveMotion{ ax=0.01, ay=-0.02, ... }"
    QString toString() const;

private:
    QSharedPointer<const DataSchema> m_schema;
    QVector<DataValue> m_values;
    QByteArray m_raw;
    qint64 m_hostTimeMs = 0;
};

Q_DECLARE_METATYPE(DataFrame)
