#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSharedPointer>

#include "models/datapacket.h"
#include "models/dataschema.h"
#include "models/loglevel.h"

// Разбирает поток байт на кадры по набору схем. Знает только про
// фрейминг и про порядок полей внутри кадра - ни про устройство,
// ни про транспорт, ни про то, как кадры потом используются.
//
// Режимы (FramingMode, выбираются в редакторе сообщений):
//   FixedLength  [typeId:u8][payload]              длина из схемы
//   HeaderLength [typeId:u8][len:u16][payload]     длина из кадра
class FrameParser : public QObject
{
    Q_OBJECT

public:
    explicit FrameParser(QObject *parent = nullptr);

    void setSchemas(const QList<DataSchema> &schemas);
    QList<DataSchema> schemas() const;
    bool hasSchema(quint8 typeId) const;

    // Дописать сырые байты в буфер и разобрать всё, что накопилось.
    void appendData(const QByteArray &data);
    void reset();

    int maxBufferSize() const;
    void setMaxBufferSize(int bytes);

    int unknownTypeCount() const;
    int crcErrorCount() const;
    int droppedFrameCount() const;
    int bufferedBytes() const;

signals:
    void frameDecoded(const DataFrame &frame);
    void frameDropped(const QString &reason);
    void logMessage(LogLevel level, const QString &text);

private:
    struct Entry {
        QSharedPointer<const DataSchema> schema;
        int fixedSize = 0;     // полный размер кадра для FixedLength
        int minPayload = 0;   // минимальный размер полезной части
    };

    void process();
    void decodeAndEmit(const Entry &entry, const QByteArray &payload, qint64 receivedMs);

    QByteArray m_buffer;
    QHash<quint8, Entry> m_byTypeId;
    int m_maxBufferSize = 1 << 20;
    int m_unknownTypes = 0;
    int m_crcErrors = 0;
    int m_dropped = 0;
};
