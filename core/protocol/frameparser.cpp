#include "core/protocol/frameparser.h"

#include <QDateTime>

#include "models/imuadapter.h"

FrameParser::FrameParser(QObject *parent)
    : QObject(parent)
{
}

void FrameParser::setSchemas(const QList<DataSchema> &schemas) {
    m_byTypeId.clear();

    for (const DataSchema &schema : schemas) {
        if (schema.isValid() == false) {
            emit logMessage(LogLevel::Error,
                            QStringLiteral("FrameParser: invalid schema \"%1\"").arg(schema.name));
            continue;
        }
        if (m_byTypeId.contains(schema.typeId)) {
            emit logMessage(LogLevel::Error,
                            QStringLiteral("FrameParser: duplicate type id %1, \"%2\" ignored")
                                .arg(schema.typeId).arg(schema.name));
            continue;
        }
        if (schema.framing == FramingMode::FixedLength && schema.hasVariableFields()) {
            emit logMessage(LogLevel::Error,
                            QStringLiteral("FrameParser: \"%1\" has variable fields but uses fixed framing")
                                .arg(schema.name));
            continue;
        }

        Entry entry;
        entry.schema = QSharedPointer<const DataSchema>(new DataSchema(schema));
        entry.fixedSize = schema.totalSize();
        entry.minPayload = schema.minPayloadSize();
        m_byTypeId.insert(schema.typeId, entry);
    }
}

QList<DataSchema> FrameParser::schemas() const {
    QList<DataSchema> result;
    for (const Entry &entry : m_byTypeId) {
        result.append(*entry.schema);
    }
    return result;
}

bool FrameParser::hasSchema(quint8 typeId) const {
    return m_byTypeId.contains(typeId);
}

void FrameParser::reset() {
    m_buffer.clear();
}

int FrameParser::maxBufferSize() const {
    return m_maxBufferSize;
}

void FrameParser::setMaxBufferSize(int bytes) {
    m_maxBufferSize = bytes < 1024 ? 1024 : bytes;
}

int FrameParser::unknownTypeCount() const {
    return m_unknownTypes;
}

int FrameParser::crcErrorCount() const {
    return m_crcErrors;
}

int FrameParser::droppedFrameCount() const {
    return m_dropped;
}

int FrameParser::bufferedBytes() const {
    return m_buffer.size();
}

void FrameParser::appendData(const QByteArray &data) {
    m_buffer.append(data);

    if (m_buffer.size() > m_maxBufferSize) {
        // Поток не разбирается - сбрасываем накопленное, чтобы не расти
        // бесконечно, и сообщаем об этом один раз.
        const int overflow = m_buffer.size() - m_maxBufferSize;
        m_buffer.remove(0, overflow);
        ++m_dropped;
        emit logMessage(LogLevel::Error,
                        QStringLiteral("FrameParser: buffer overflow, dropped %1 bytes").arg(overflow));
    }

    process();
}

void FrameParser::process() {
    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    while (m_buffer.size() >= 1) {
        const quint8 typeId = static_cast<quint8>(m_buffer.at(0));
        const auto lookup = m_byTypeId.constFind(typeId);
        if (lookup == m_byTypeId.constEnd()) {
            ++m_unknownTypes;
            ++m_dropped;
            // Синхронизация потеряна: сдвигаемся на байт и ищем дальше.
            m_buffer.remove(0, 1);
            continue;
        }

        const Entry &entry = lookup.value();
        int total = 0;

        if (entry.schema->framing == FramingMode::HeaderLength) {
            if (m_buffer.size() < 3) {
                break;
            }
            const int declared = static_cast<quint8>(m_buffer.at(1))
                               | (static_cast<quint8>(m_buffer.at(2)) << 8);
            if (declared < entry.minPayload) {
                ++m_dropped;
                m_buffer.remove(0, 1);
                emit logMessage(LogLevel::Warning,
                                QStringLiteral("FrameParser: type %1 declares %2 bytes, schema needs %3")
                                    .arg(typeId).arg(declared).arg(entry.minPayload));
                continue;
            }
            total = 3 + declared;
        } else {
            total = entry.fixedSize;
        }

        if (m_buffer.size() < total) {
            break;
        }

        const QByteArray frame = m_buffer.left(total);
        m_buffer.remove(0, total);
        decodeAndEmit(entry, frame.mid(entry.schema->headerSize()), now);
    }
}

void FrameParser::decodeAndEmit(const Entry &entry, const QByteArray &payload, qint64 receivedMs) {
    const DataSchema &schema = *entry.schema;

    QString error;
    DataFrame frame = DataFrame::decode(entry.schema, payload, receivedMs, &error);
    if (error.isEmpty() == false) {
        ++m_dropped;
        emit frameDropped(QStringLiteral("\"%1\": %2").arg(schema.name, error));
        return;
    }

    // Контрольная сумма считается по байтам до crc-поля.
    if (frame.hasCrcField()) {
        const quint16 got = frame.crc16();
        const quint16 expected = ImuAdapter::crc16Ccitt(frame.crcCoveredBytes());
        if (expected != got) {
            ++m_crcErrors;
            ++m_dropped;
            emit logMessage(LogLevel::Warning,
                            QStringLiteral("CRC mismatch in \"%1\": got 0x%2, expected 0x%3")
                                .arg(schema.name)
                                .arg(got, 4, 16, QLatin1Char('0'))
                                .arg(expected, 4, 16, QLatin1Char('0')));
            emit frameDropped(QStringLiteral("CRC mismatch"));
        }
    }

    emit frameDecoded(frame);
}
