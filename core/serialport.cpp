#include "core/serialport.h"
#include "models/packettype.h"
#include <QDebug>
#include <cstring>
#include <cstdint>

SerialPort::SerialPort(QObject *parent)
    : QObject(parent)
{
    serial = new QSerialPort(this);

    connect(serial, &QSerialPort::readyRead,
            this, &SerialPort::readData);

    connect(serial, &QSerialPort::errorOccurred,
            this, &SerialPort::handleError);
}

SerialPort::~SerialPort() {
    serial->close();
}

void SerialPort::openPort(const PortConfig &config) {
    setState(ConnectionState::Connecting);

    serial->setPortName(config.name);
    serial->setBaudRate(config.baud);
    if (!!serial->open(QIODevice::ReadOnly)) {
        emit logMessage(LogLevel::Info,
                        QString("Port %1 opened at %2 baud")
                            .arg(config.name)
                            .arg(config.baud));
        setState(ConnectionState::Connected);
    }
    else {
        emit logMessage(LogLevel::Error,
                        QString("Failed to open port %1").arg(config.name));
        setState(ConnectionState::Disconnected);
    }
}

void SerialPort::closePort() {
    if(serial->isOpen()) {
        serial->close();
        emit logMessage(LogLevel::Info, "Port closed");
    }

    setState(ConnectionState::Disconnected);
}

bool SerialPort::connectStatus() {
      return currentState == ConnectionState::Connected;
}

ConnectionState SerialPort::state() const {
    return currentState;
}

QStringList SerialPort::updatePortList() {
    QStringList ports;
    const auto serialPorts = QSerialPortInfo::availablePorts();
    int i = 0;
    for (const QSerialPortInfo &port : serialPorts) {
        ports << port.portName();
        i++;
    }
    QString string = QString("Searched %1 ports").arg(i);
    emit logMessage(LogLevel::Info, string);
    return ports;
}

void SerialPort::setState(ConnectionState state) {
    if (currentState == state)
        return;

    currentState = state;
    emit connectionChanged(state);
}

void SerialPort::readData() {
    buffer.append(serial->readAll());

    while (buffer.size() >= 1) {
        quint8 type = static_cast<quint8>(buffer.at(0));
        int len = frameLength(type);

        if (len < 0) {
            buffer.remove(0, 1);
            emit logMessage(LogLevel::Warning,
                            QString("Unknown packet type: %1").arg(type));
            continue;
        }

        if (buffer.size() < len) {
            break;
        }

        QByteArray frame = buffer.left(len);
        buffer.remove(0, len);

        switch (type) {
        case static_cast<quint8>(PacketType::LiveMotion): {
            MotionPacket packet;
            if (!parseMotionPacket(frame, packet)) { continue; }
            emit motionPacketReceived(packet);
            break;
        }
        case static_cast<quint8>(PacketType::SegmentEnd): {
            SegmentEndPacket packet;
            if (!parseSegmentEnd(frame, packet)) { continue; }
            emit segmentEndReceived(packet);
            break;
        }
        case static_cast<quint8>(PacketType::Temperature):
            emit logMessage(LogLevel::Debug, "Temperature packet received");
            break;
        default:
            emit logMessage(LogLevel::Warning,
                            QString("Unhandled packet type: %1").arg(type));
            break;
        }
    }
}

int SerialPort::frameLength(quint8 type) const {
    switch (type) {
    case static_cast<quint8>(PacketType::LiveMotion):
        return 1 + static_cast<int>(sizeof(MotionSample)) + 1;
    case static_cast<quint8>(PacketType::SegmentEnd):
        return 5;
    default:
        return -1;
    }
}

void SerialPort::handleError(QSerialPort::SerialPortError error) {
    if (error == QSerialPort::ResourceError) {
        emit logMessage(LogLevel::Error, "Serial port error, device disconnected");
        serial->close();
        setState(ConnectionState::Disconnected);
        emit portDisconnected();
    }
}

bool SerialPort::parseMotionPacket(const QByteArray &frame, MotionPacket &packet) {
    const int sampleSize = static_cast<int>(sizeof(MotionSample));
    if (frame.size() < 1 + sampleSize + 1) {
        return false;
    }

    std::memcpy(&packet.sample, frame.data() + 1, sampleSize);
    packet.recording = (frame.at(1 + sampleSize) != 0);

    return true;
}

bool SerialPort::parseSegmentEnd(const QByteArray &frame, SegmentEndPacket &packet) {
    if (frame.size() < 5) {
        return false;
    }

    uint16_t count = 0;
    uint16_t crc   = 0;
    std::memcpy(&count, frame.data() + 1, 2);
    std::memcpy(&crc,   frame.data() + 3, 2);

    packet.count = count;
    packet.crc16 = crc;

    return true;
}
