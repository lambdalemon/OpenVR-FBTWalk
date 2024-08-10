#include "move.h"

#include <openvr.h>
#include <thread>
#include <chrono>
#include <math.h>

#include "fbtwalk_context.h"

using namespace std::chrono_literals;

struct PoseCha {
    float cha; // Moving distance
    int walk_zt = 0; //0 = Still  1 = forward 2 = backward
    int cont = 0; // Movement count, greater than 450 equals stop
    float p_dis = 0;
    float s_x = 0;
    float s_z = 0;
    float s_i = 0;
    bool NewSteap = true;
    float R_dis = 0;
    float HTdis = 0;
    bool walkb = false;
};

static PoseCha PchaL;
static PoseCha PchaR;
static vr::HmdMatrix34_t orig;
static vr::HmdMatrix34_t _ORIG;

static int UGOKU3_zt = 0; // 0 = Still  1 = forward 2 = backward

void Move_3(PoseCha& Pcha, const vr::HmdMatrix34_t& nowpose, const vr::HmdMatrix34_t& lastpose, const vr::HmdMatrix34_t& pointC) {

    Pcha.walkb = false;
    Pcha.cha =
        sqrt((lastpose.m[0][3] - nowpose.m[0][3]) * (lastpose.m[0][3] - nowpose.m[0][3]) +
            (lastpose.m[2][3] - nowpose.m[2][3]) * (lastpose.m[2][3] - nowpose.m[2][3]));
    if (Pcha.cha > 0.3) return;

    float distance_ima =
        sqrt((nowpose.m[0][3] - pointC.m[0][3]) * (nowpose.m[0][3] - pointC.m[0][3]) +
            (nowpose.m[2][3] - pointC.m[2][3]) * (nowpose.m[2][3] - pointC.m[2][3]));

    float distance_mae =
        sqrt((lastpose.m[0][3] - pointC.m[0][3]) * (lastpose.m[0][3] - pointC.m[0][3]) +
            (lastpose.m[2][3] - pointC.m[2][3]) * (lastpose.m[2][3] - pointC.m[2][3]));

    float pdis = distance_ima - distance_mae;
    Pcha.p_dis += pdis;
    bool tracker_moved = Pcha.cha > ctx.config.limit*2;

    if (abs(Pcha.p_dis) > 0.03 && tracker_moved) {
        Pcha.p_dis = 0;
        Pcha.cont = 0;
        if (UGOKU3_zt == 0) {
            UGOKU3_zt = ctx.config.back && pdis < 0 ? 2 : 1;
        }
    } else {
        Pcha.cont++;
        if (Pcha.cont > 450) {
            Pcha.walk_zt = 0;
            if (UGOKU3_zt != 0 && PchaL.walk_zt == 0 && PchaR.walk_zt == 0) {
                UGOKU3_zt = 0;
                printf("move3_Still\n");
            }
        }
    }

    if (UGOKU3_zt == 0 || !tracker_moved) return;

    Pcha.walk_zt = UGOKU3_zt;
    if ((pdis < 0) == (UGOKU3_zt == 1)) {
        Pcha.HTdis = 0;
        if (Pcha.cha < PchaR.cha && PchaR.walkb || Pcha.cha < PchaL.cha && PchaL.walkb) return;
        if (Pcha.NewSteap) {
            Pcha.R_dis += pdis;
            if (abs(Pcha.R_dis) > 0.25) {
                Pcha.NewSteap = false;
                ctx.walk_record.steps++;
            }
        }
        ctx.walk_record.distance += Pcha.cha;
        Pcha.walkb = true;

        Pcha.s_x += nowpose.m[0][3] - lastpose.m[0][3];
        Pcha.s_z += nowpose.m[2][3] - lastpose.m[2][3];
        Pcha.s_i++;
        if (ctx.config.smooth) {
            orig.m[0][3] += (Pcha.s_x / Pcha.s_i) * ((float)ctx.config.speed / 100);
            orig.m[2][3] += (Pcha.s_z / Pcha.s_i) * ((float)ctx.config.speed / 100);
        } else {
            orig.m[0][3] += (nowpose.m[0][3] - lastpose.m[0][3]) * ((float)ctx.config.speed / 100);
            orig.m[2][3] += (nowpose.m[2][3] - lastpose.m[2][3]) * ((float)ctx.config.speed / 100);
        }
        vr::VRChaperoneSetup()->SetWorkingStandingZeroPoseToRawTrackingPose(&orig);
        vr::VRChaperoneSetup()->ShowWorkingSetPreview();
    } else {
        Pcha.HTdis += Pcha.cha;
        if (Pcha.HTdis > 0.2) {
            if (!Pcha.NewSteap) {
                Pcha.NewSteap = true;
                Pcha.R_dis = 0;
            }
            if (Pcha.s_i != 0) {
                Pcha.s_i = 0;
                Pcha.s_x = 0;
                Pcha.s_z = 0;
            }
        }
    }
}

void ResetPosition() {
    vr::VRChaperoneSetup()->RevertWorkingCopy();
    vr::VRChaperoneSetup()->GetWorkingStandingZeroPoseToRawTrackingPose(&orig);
    printf("Reset\n");
    _ORIG.m[1][3] = orig.m[1][3];
    orig = _ORIG;
    vr::VRChaperoneSetup()->SetWorkingStandingZeroPoseToRawTrackingPose(&orig);
    vr::VRChaperoneSetup()->ShowWorkingSetPreview();
}

void InitOrigin() {
	bool hmdconnected = false;
    while (!hmdconnected) {
        hmdconnected = vr::VRSystem()->IsTrackedDeviceConnected(0);
        std::this_thread::sleep_for(500ms);
    }
    printf("SteamVR_HMD->ready\n");

    bool orig_ok = false;
    while (!orig_ok) {
        orig_ok = vr::VRChaperoneSetup()->GetWorkingStandingZeroPoseToRawTrackingPose(&_ORIG);
        std::this_thread::sleep_for(500ms);
    }
    printf("orig  X =  %.5f, Y = %.5f\n", _ORIG.m[0][3], _ORIG.m[2][3]);
}

void pose_loop() {
    vr::TrackedDevicePose_t devicePose[vr::k_unMaxTrackedDeviceCount];

    PchaL.walk_zt = 0;
    PchaR.walk_zt = 0;

    vr::HmdMatrix34_t lastposeR;
    vr::HmdMatrix34_t lastposeL;

    vr::VRChaperoneSetup()->RevertWorkingCopy();
    vr::VRChaperoneSetup()->GetWorkingStandingZeroPoseToRawTrackingPose(&orig);

    printf("orig  X =  %.5f, Y = %.5f\n", orig.m[0][3], orig.m[2][3]);

    while (ctx.start)
    {
        std::this_thread::sleep_for(2ms);

        if (ctx.want_position_reset) {
            ResetPosition();
            ctx.want_position_reset = false;
        }

        if (ctx.config.treadmill && !ctx.config._POINTC.ready) continue;

        vr::VRSystem()->GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin::TrackingUniverseRawAndUncalibrated, 0, devicePose, vr::k_unMaxTrackedDeviceCount);

        vr::HmdMatrix34_t pointC = devicePose[0].mDeviceToAbsoluteTracking;
        if (ctx.config.treadmill) {
            pointC.m[0][3] = ctx.config._POINTC.x;
            pointC.m[2][3] = ctx.config._POINTC.z;
        } else {
            pointC.m[0][3] = pointC.m[0][3] + 1.5 * pointC.m[0][2];
            pointC.m[2][3] = pointC.m[2][3] + 1.5 * pointC.m[2][2];
        }

        const auto foot_update = [&pointC] (PoseCha& Pcha, const vr::HmdMatrix34_t& nowpose, vr::HmdMatrix34_t& lastpose) {
            Move_3(Pcha, nowpose, lastpose, pointC);
            lastpose = nowpose;
        };
        foot_update(PchaL, devicePose[ctx.tracker.footL].mDeviceToAbsoluteTracking, lastposeL);
        foot_update(PchaR, devicePose[ctx.tracker.footR].mDeviceToAbsoluteTracking, lastposeR);
    }

    printf("pose_loop->exit\n");
}

