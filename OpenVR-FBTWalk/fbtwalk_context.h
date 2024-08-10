#pragma once

#include <openvr.h>

struct Config {
    bool treadmill = false;
    int speed = 100;
    float limit = 0.0004;
    struct PointC {
        float x = 0;
        float z = 0;
        bool ready = false;
    } _POINTC;
    bool smooth = true;
    bool back = false;
};

struct Trackers {
    bool ok = false;
    vr::TrackedDeviceIndex_t waist = -1;
    vr::TrackedDeviceIndex_t footL = -1;
    vr::TrackedDeviceIndex_t footR = -1;
};

struct WalkRecord {
    int steps = 0;
    float distance = 0;
};

struct FbtWalkContext {
    Config config;
    bool start = false;
    Trackers tracker;
    WalkRecord walk_record;
    bool want_position_reset = false;
};

inline FbtWalkContext ctx;
