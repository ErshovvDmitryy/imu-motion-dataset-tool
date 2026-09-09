#pragma once

#include <QString>

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
