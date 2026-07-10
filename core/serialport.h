#ifndef SERIALPORT_H
#define SERIALPORT_H



#include <QObject>
#include <QSerialPort>
#include <QStringList>
#include <QDebug>
#include <QSerialPortInfo>

enum class ConnectionState
{
    Disconnected,
    Connecting,
    Connected
};

struct MotionSample
{
    float ax;
    float ay;
    float az;

    float gx;
    float gy;
    float gz;

    uint32_t time;
};


class SerialPort : public QObject
{
    Q_OBJECT

public:
    explicit SerialPort(QObject *parent = nullptr);
    ~SerialPort();

    void openPort(const QString& portName, int baudRate);
    void closePort();

    bool connectStatus();

    QStringList updatePortList();

signals:
    void dataReceived(const MotionSample& data);
    void portDisconnected();

private slots:
    void readData();
    void handleError(QSerialPort::SerialPortError error);


private:
    QSerialPort *serial;
    QByteArray buffer;

};

#endif // SERIALPORT_H
