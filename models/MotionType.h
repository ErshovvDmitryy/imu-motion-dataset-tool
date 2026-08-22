#pragma once

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
