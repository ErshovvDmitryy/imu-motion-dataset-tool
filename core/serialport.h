#pragma once

#include <QByteArray>
#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>

#include "models/loglevel.h"
#include "models/portconfig.h"
#include "models/statusport.h"

// Только транспорт: открывает порт и отдаёт поток байт как есть.
// Разбором потока занимается MessageDecoder - так протокол можно менять
// без правок этого класса.
class SerialPort : public QObject
{
    Q_OBJECT

public:
    explicit SerialPort(QObject *parent = nullptr);
    ~SerialPort() override;

    void openPort(const PortConfig &config);
    void closePort();

    bool connectStatus() const;
    ConnectionState state() const;
    QString portName() const;

    QStringList updatePortList();

    quint64 bytesReceived() const;

signals:
    void rawDataReceived(const QByteArray &data);
    void portDisconnected();
    void connectionChanged(ConnectionState state);
    void logMessage(LogLevel level, const QString &text);

private slots:
    void readData();
    void handleError(QSerialPort::SerialPortError error);

private:
    void setState(ConnectionState state);

    QSerialPort *m_serial;
    ConnectionState m_state = ConnectionState::Disconnected;
    quint64 m_bytes = 0;
};
