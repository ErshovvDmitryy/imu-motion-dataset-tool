#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

#include "core/protocol/frameparser.h"
#include "models/datapacket.h"
#include "models/dataschema.h"
#include "models/loglevel.h"

// Тонкая прослойка над FrameParser: удобная точка подключения транспорта
// (SerialPort, файл, сокет) к потребителям кадров и накопитель статистики.
class MessageDecoder : public QObject
{
    Q_OBJECT

public:
    explicit MessageDecoder(QObject *parent = nullptr);

    void setSchemas(const QList<DataSchema> &schemas);
    QList<DataSchema> schemas() const;

    // Единственная точка входа: сюда льются сырые байты транспорта.
    void onRawData(const QByteArray &data);
    void reset();

    quint64 totalFrames() const;
    quint64 framesOfType(quint8 typeId) const;
    QHash<quint8, quint64> frameStats() const;

    int crcErrorCount() const;
    int droppedFrameCount() const;
    int unknownTypeCount() const;

signals:
    void frameReceived(const DataFrame &frame);
    void logMessage(LogLevel level, const QString &text);

private slots:
    void onFrameDecoded(const DataFrame &frame);

private:
    FrameParser *m_parser;
    QHash<quint8, quint64> m_stats;
    quint64 m_total = 0;
};
