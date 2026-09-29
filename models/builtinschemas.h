#pragma once

#include <QList>
#include <QString>

#include "models/dataschema.h"

// Встроенные типы сообщений. Описаны обычными схемами, поэтому парсер
// не знает ничего про IMU: это просто два .qproto, которые создаются при
// первом запуске, если папки protocols/ ещё нет.
//
// Раскладка полей в точности повторяет старую структуру MotionSample,
// поэтому прошивка ESP32 работает без изменений:
//   LiveMotion  = [106][ax ay az gx gy gz : float32][time : uint32][recording : bool]  = 30 байт
//   SegmentEnd  = [107][count : uint16][crc16 : uint16]                                 = 5 байт

namespace BuiltinSchemas {

constexpr const char *imuName     = "LiveMotion";
constexpr const char *segmentName = "SegmentEnd";

namespace ImuField {
constexpr const char *ax        = "ax";
constexpr const char *ay        = "ay";
constexpr const char *az        = "az";
constexpr const char *gx        = "gx";
constexpr const char *gy        = "gy";
constexpr const char *gz        = "gz";
constexpr const char *time      = "time";
constexpr const char *recording = "recording";
} // namespace ImuField

namespace SegmentField {
constexpr const char *count = "count";
constexpr const char *crc16 = "crc16";
} // namespace SegmentField

DataSchema imuLiveMotion();
DataSchema imuSegmentEnd();

QList<DataSchema> all();

} // namespace BuiltinSchemas
