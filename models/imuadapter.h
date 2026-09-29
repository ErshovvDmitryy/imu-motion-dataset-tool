#pragma once

#include <QByteArray>
#include <QSharedPointer>
#include <QString>
#include <QVector>

#include "models/datapacket.h"
#include "models/dataschema.h"
#include "models/motionsample.h"

// Мост между общей моделью DataFrame и унаследованной IMU-подсистемой
// (MotionRecorder -> DatabaseManager). Здесь и только здесь живёт знание
// о раскладке MotionSample.
//
// Запись произвольных схем в базу - отдельная задача (DatabaseManager
// сейчас жёстко заточен под samples/motion_data), поэтому в БД попадают
// только кадры встроенной схемы IMU.

namespace ImuAdapter {

// CRC-16/CCITT-FALSE: полином 0x1021, init 0xFFFF, без рефлексии и XOR.
quint16 crc16Ccitt(const QByteArray &data);

// Сколько байт одного кадра IMU попадает в CRC сегмента.
// Прошивка считает сумму по 28 байтам (6 float + time) и не включает
// байт recording, поэтому отбрасываем хвост.
int imuCrcCoveredSize();

bool isImuFrame(const DataFrame &frame);

bool dataFrameToMotionSample(const DataFrame &frame, MotionSample *sample);
bool dataFrameToTimestampUs(const DataFrame &frame, quint32 *timestampUs);
bool dataFrameToRecording(const DataFrame &frame, bool *recording);
bool dataFrameToSegmentCount(const DataFrame &frame, quint16 *count);
bool dataFrameToSegmentCrc(const DataFrame &frame, quint16 *crc);

// Обратное направление. Нужно странице базы данных: она читает строки
// SQL и хочет отдать их графикам, которые работают с кадрами.
QSharedPointer<const DataSchema> imuSchemaPtr();
DataFrame makeImuFrame(float ax, float ay, float az, float gx, float gy, float gz,
                       quint32 timeUs, bool recording);
QVector<DataFrame> motionSamplesToFrames(const QVector<MotionSample> &samples);

} // namespace ImuAdapter
