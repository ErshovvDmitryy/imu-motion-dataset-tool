#pragma once

#include <QString>
#include <QVector>

enum class MotionType : int {
    DoubleTap = 0,

    SwipeLeft,
    SwipeRight,

    SwipeUp,
    SwipeDown,

    CircleCW,
    CircleCCW,

    Shake,

    NormalHandMovement,
    Walking,

    Unlabeled,
    Unknown
};

inline QString motionTypeToString(int motionTypeId) {
    const MotionType type = static_cast<MotionType>(motionTypeId);
    switch (type) {
        case MotionType::DoubleTap:          return "DoubleTap";
        case MotionType::SwipeLeft:          return "SwipeLeft";
        case MotionType::SwipeRight:         return "SwipeRight";
        case MotionType::SwipeUp:            return "SwipeUp";
        case MotionType::SwipeDown:          return "SwipeDown";
        case MotionType::CircleCW:           return "CircleCW";
        case MotionType::CircleCCW:          return "CircleCCW";
        case MotionType::Shake:              return "Shake";
        case MotionType::NormalHandMovement: return "NormalHandMovement";
        case MotionType::Walking:            return "Walking";
        case MotionType::Unlabeled:          return "Unlabeled";
        case MotionType::Unknown:            return "Unknown";
        default:                             return "Invalid";
    }
}

inline QVector<QVector<QVector<float>>> sliceWindows(
    const QVector<QVector<float>> &data,
    int windowSize,
    int windowBias,
    float minRemainingRatio = 0.1f)
{
    QVector<QVector<QVector<float>>> windows;

    int totalSamples = data.size();
    if (totalSamples < windowSize) {
        return windows;
    }

    int lastStart = 0;
    for (int start = 0; start + windowSize <= totalSamples; start += windowBias) {
        windows.append(data.mid(start, windowSize));
        lastStart = start;
    }

    int lastWindowStart = totalSamples - windowSize;
    int remaining = totalSamples - (lastStart + windowSize);

    if (remaining > 0) {
        int minRemaining = static_cast<int>(windowSize * minRemainingRatio);

        if (remaining >= minRemaining) {
            auto lastWindow = data.mid(lastWindowStart, windowSize);
            windows.append(lastWindow);

        }
    }

    return windows;
}
