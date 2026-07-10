#include "serialport.h"


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

bool SerialPort::connectStatus()
{
      return serial->isOpen();
}

QStringList SerialPort::updatePortList()
{
    QStringList ports;
    const auto serialPorts = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : serialPorts) {
        ports << port.portName();
    }
    return ports;
}

void SerialPort::readData()
{
    //qDebug() << "start readData";
    buffer.append(serial->readAll());

    while(buffer.contains('\n'))
    {
        //qDebug() << "readData in while";
        int index = buffer.indexOf('\n');
        QByteArray line = buffer.left(index).trimmed();
        buffer.remove(0, index + 1);

        QString str(line);
        QStringList values = str.split(',');
        //qDebug() << "step to if";

        if(values.size() == 7)
        {
            MotionSample data;
            data.ax = values[0].toDouble();
            //qDebug() << data.ax;
            data.ay = values[1].toDouble();
            //qDebug() << data.ay;
            data.az = values[2].toDouble();
            //qDebug() << data.az;
            data.gx = values[3].toDouble();
            //qDebug() << data.gx;
            data.gy = values[4].toDouble();
            //qDebug() << data.gy;
            data.gz = values[5].toDouble();
            //qDebug() << data.gz;
            data.time = values[6].toUInt();

            emit dataReceived(data);
        }
    }
}

void SerialPort::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError)
    {
        qDebug() << "Все упало нахуй";
        serial->close();
        emit portDisconnected();
    }
}
