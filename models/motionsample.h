#pragma once

#include <QVector>

struct MotionSample {

    QVector<float> accel;   // ax, ay, az
    QVector<float> gyro;    // gx, gy, gz

    float ax;
    float ay;
    float az;

    float gx;
    float gy;
    float gz;

    uint32_t time;
};

struct MotionPacket {
    MotionSample sample;
    bool recording;
};

struct SegmentEndPacket {
    uint16_t count;
    uint16_t crc16;
};
