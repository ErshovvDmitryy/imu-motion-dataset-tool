#include "core/serialport.h"

SerialPort::SerialPort(QObject *parent)
    : QObject(parent)
{
    serial = new QSerialPort(this);

    connect(serial, &QSerialPort::readyRead,
            this, &SerialPort::readData);

    connect(serial, &QSerialPort::errorOccurred,
            this, &SerialPort::handleError);
}

SerialPort::~SerialPort()
{

    serial->close();
}

void SerialPort::openPort(const PortConfig &config)
{
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

void SerialPort::closePort()
{
    if(serial->isOpen()) {
        serial->close();
        emit logMessage(LogLevel::Info, "Port closed");
    }

    setState(ConnectionState::Disconnected);
}

bool SerialPort::connectStatus()
{
      return currentState == ConnectionState::Connected;
}

ConnectionState SerialPort::state() const
{
    return currentState;
}

QStringList SerialPort::updatePortList()
{
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

void SerialPort::setState(ConnectionState state)
{
    if (currentState == state)
        return;

    currentState = state;
    emit connectionChanged(state);
}

void SerialPort::readData() {
    buffer.append(serial->readAll());

    while (buffer.contains('\n'))
    {
        int index = buffer.indexOf('\n');

        QByteArray line = buffer.left(index).trimmed();
        buffer.remove(0, index + 1);

        QString str(line);

        QStringList values = str.split(',');

        if (values.isEmpty())
            continue;

        PacketType type =
            static_cast<PacketType>(values[0].toInt());

        switch(type) {

        case PacketType::LiveMotion:
        {
            MotionPacket packet;
            if (!parseMotionPacket(values, packet)) { continue; }
            emit motionPacketReceived(packet);
            break;
        }

        case PacketType::SegmentEnd:
        {
            SegmentEndPacket packet;
            if (!parseSegmentEnd(values, packet)) { continue; }
            emit segmentEndReceived(packet);
            break;
        }

        case PacketType::Temperature:

            emit logMessage(LogLevel::Debug, "Temperature packet received");
            break;

        default:

            emit logMessage(LogLevel::Warning,
                            QString("Unknown packet type: %1")
                                .arg(values[0]));
            break;

        }
    }
}

void SerialPort::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError)
    {
        emit logMessage(LogLevel::Error, "Serial port error, device disconnected");
        serial->close();
        setState(ConnectionState::Disconnected);
        emit portDisconnected();
    }
}

bool SerialPort::parseMotionPacket(const QStringList &values,
                                   MotionPacket &packet)
{
    if (values.size() < 9)
        return false;

    packet.sample.ax   = values[1].toFloat();
    packet.sample.ay   = values[2].toFloat();
    packet.sample.az   = values[3].toFloat();

    packet.sample.gx   = values[4].toFloat();
    packet.sample.gy   = values[5].toFloat();
    packet.sample.gz   = values[6].toFloat();

    packet.sample.time = values[7].toUInt();

    packet.recording   = values[8].toInt() != 0;

    return true;
}

bool SerialPort::parseSegmentEnd(const QStringList &values,
                                 SegmentEndPacket &packet)
{
    if (values.size() < 3)
        return false;

    packet.count = values[1].toUInt();
    packet.crc16 = static_cast<uint16_t>(values[2].toUInt(nullptr, 16));

    return true;
}
