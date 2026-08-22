#pragma once

#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include <QSerialPortInfo>
#include <models/motionsample.h>
#include <models/packettype.h>
#include <models/portconfig.h>
#include <models/statusport.h>
#include <models/loglevel.h>

class SerialPort : public QObject
{
    Q_OBJECT

public:
    explicit SerialPort(QObject *parent = nullptr);
    ~SerialPort();

    void openPort(const PortConfig &config);
    void closePort();

    bool connectStatus();
    ConnectionState state() const;

    QStringList updatePortList();

signals:
    void motionPacketReceived(const MotionPacket &packet);

    void segmentEndReceived(const SegmentEndPacket &packet);

    void portDisconnected();

    void connectionChanged(ConnectionState state);

    void logMessage(LogLevel level, const QString &text);

private slots:
    void readData();
    void handleError(QSerialPort::SerialPortError error);

private:
    void setState(ConnectionState state);

    QSerialPort *serial;
    QByteArray buffer;
    ConnectionState currentState = ConnectionState::Disconnected;

    bool parseMotionPacket(const QStringList &values, MotionPacket &packet);
    bool parseSegmentEnd(const QStringList &values, SegmentEndPacket &packet);
};