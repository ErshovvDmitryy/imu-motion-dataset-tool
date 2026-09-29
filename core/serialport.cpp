#include "core/serialport.h"

#include <QDebug>

SerialPort::SerialPort(QObject *parent)
    : QObject(parent)
    , m_serial(new QSerialPort(this))
{
    connect(m_serial, &QSerialPort::readyRead, this, &SerialPort::readData);
    connect(m_serial, &QSerialPort::errorOccurred, this, &SerialPort::handleError);
}

SerialPort::~SerialPort()
{
    m_serial->close();
}

void SerialPort::openPort(const PortConfig &config) {
    setState(ConnectionState::Connecting);

    m_serial->setPortName(config.name);
    m_serial->setBaudRate(config.baud);
    if (m_serial->open(QIODevice::ReadOnly)) {
        emit logMessage(LogLevel::Info,
                        QString("Port %1 opened at %2 baud").arg(config.name).arg(config.baud));
        setState(ConnectionState::Connected);
    } else {
        emit logMessage(LogLevel::Error,
                        QString("Failed to open port %1").arg(config.name));
        setState(ConnectionState::Disconnected);
    }
}

void SerialPort::closePort() {
    if (m_serial->isOpen()) {
        m_serial->close();
        emit logMessage(LogLevel::Info, "Port closed");
    }
    setState(ConnectionState::Disconnected);
}

bool SerialPort::connectStatus() const {
    return m_state == ConnectionState::Connected;
}

ConnectionState SerialPort::state() const {
    return m_state;
}

QString SerialPort::portName() const {
    return m_serial->portName();
}

quint64 SerialPort::bytesReceived() const {
    return m_bytes;
}

QStringList SerialPort::updatePortList() {
    QStringList ports;
    const auto serialPorts = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : serialPorts) {
        ports << port.portName();
    }
    emit logMessage(LogLevel::Info, QString("Searched %1 ports").arg(ports.size()));
    return ports;
}

void SerialPort::setState(ConnectionState state) {
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit connectionChanged(state);
}

void SerialPort::readData() {
    const QByteArray chunk = m_serial->readAll();
    if (chunk.isEmpty()) {
        return;
    }
    m_bytes += static_cast<quint64>(chunk.size());
    emit rawDataReceived(chunk);
}

void SerialPort::handleError(QSerialPort::SerialPortError error) {
    if (error == QSerialPort::ResourceError) {
        emit logMessage(LogLevel::Error, "Serial port error, device disconnected");
        m_serial->close();
        setState(ConnectionState::Disconnected);
        emit portDisconnected();
    }
}
