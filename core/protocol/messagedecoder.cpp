#include "core/protocol/messagedecoder.h"

MessageDecoder::MessageDecoder(QObject *parent)
    : QObject(parent)
    , m_parser(new FrameParser(this))
{
    connect(m_parser, &FrameParser::frameDecoded,
            this, &MessageDecoder::onFrameDecoded);
    connect(m_parser, &FrameParser::logMessage,
            this, &MessageDecoder::logMessage);
    connect(m_parser, &FrameParser::frameDropped,
            this, [this](const QString &reason) {
        emit logMessage(LogLevel::Warning, QStringLiteral("Frame dropped: %1").arg(reason));
    });
}

void MessageDecoder::setSchemas(const QList<DataSchema> &schemas) {
    m_stats.clear();
    m_total = 0;
    m_parser->reset();
    m_parser->setSchemas(schemas);
}

QList<DataSchema> MessageDecoder::schemas() const {
    return m_parser->schemas();
}

void MessageDecoder::onRawData(const QByteArray &data) {
    if (data.isEmpty()) {
        return;
    }
    m_parser->appendData(data);
}

void MessageDecoder::reset() {
    m_parser->reset();
    m_stats.clear();
    m_total = 0;
}

void MessageDecoder::onFrameDecoded(const DataFrame &frame) {
    ++m_total;
    m_stats[frame.typeId()] += 1;
    emit frameReceived(frame);
}

quint64 MessageDecoder::totalFrames() const {
    return m_total;
}

quint64 MessageDecoder::framesOfType(quint8 typeId) const {
    return m_stats.value(typeId, 0);
}

QHash<quint8, quint64> MessageDecoder::frameStats() const {
    return m_stats;
}

int MessageDecoder::crcErrorCount() const {
    return m_parser->crcErrorCount();
}

int MessageDecoder::droppedFrameCount() const {
    return m_parser->droppedFrameCount();
}

int MessageDecoder::unknownTypeCount() const {
    return m_parser->unknownTypeCount();
}
