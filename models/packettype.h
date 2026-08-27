#pragma once

enum class PacketType {
    LiveMotion  = 106,
    SegmentEnd  = 107,

    Temperature = 110,
    DeviceInfo  = 111,

    Ack         = 200,
    Error       = 255,
};
