#include "ui.h"

#include <imgui/imgui.h>
#include <thread>
#include <chrono>
#include <math.h>

#include "fbtwalk_context.h"
#include "move.h"
#include "image_helpers.h"
#include "ui_translator.h"
#include "config.h"

using namespace std::chrono_literals;

enum class PageT {
    MAIN,
    OPTIONS,
    DETECT_TRACKERS,
    ASSESS_SENSITIVITY,
    LOCATE_TREADMILL
};

static PageT active_page = PageT::DETECT_TRACKERS;
static const UITranslator& tr = UITranslator::from_locale();
static int sensitivity_sample_count = -1;
constexpr int SENSITIVITY_SAMPLE_COUNT_MAX = 2500;

void tracker_update() {
    for (vr::TrackedDeviceIndex_t ids = 0; ids < vr::k_unMaxTrackedDeviceCount; ids++) {
        vr::ETrackedDeviceClass trackedDeviceClass = vr::VRSystem()->GetTrackedDeviceClass(ids);
        if (trackedDeviceClass != vr::TrackedDeviceClass_GenericTracker) continue;
        char buf[1024];
        vr::ETrackedPropertyError err;
        vr::VRSystem()->GetStringTrackedDeviceProperty(ids, vr::TrackedDeviceProperty::Prop_ControllerType_String, buf, sizeof(buf), &err);
        std::string str = buf;
        if (str.find("waist") != -1) {
            ctx.tracker.waist = ids;
        } else if (str.find("left_foot") != -1) {
            ctx.tracker.footL = ids;
        } else if (str.find("right_foot") != -1) {
            ctx.tracker.footR = ids;
        }
    }
    ctx.tracker.ok = (ctx.tracker.waist != -1 && ctx.tracker.footL != -1 && ctx.tracker.footR != -1);
}

void assess_tracker_sensitivity() {
    vr::TrackedDevicePose_t devicePose[vr::k_unMaxTrackedDeviceCount];
    vr::HmdMatrix34_t lastP;
    float s_dis = 0.f;

    for (sensitivity_sample_count = 0; sensitivity_sample_count < SENSITIVITY_SAMPLE_COUNT_MAX; sensitivity_sample_count++) {
        vr::VRSystem()->GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin::TrackingUniverseRawAndUncalibrated, 0, devicePose, vr::k_unMaxTrackedDeviceCount);
        vr::HmdMatrix34_t newP = devicePose[ctx.tracker.footL].mDeviceToAbsoluteTracking;
        if (sensitivity_sample_count > 0) {
            float dis = sqrt((lastP.m[0][3] - newP.m[0][3]) * (lastP.m[0][3] - newP.m[0][3]) +
                            (lastP.m[2][3] - newP.m[2][3]) * (lastP.m[2][3] - newP.m[2][3]));
            s_dis += dis;
            ctx.config.limit = s_dis / sensitivity_sample_count;
        }
        lastP = newP;
        std::this_thread::sleep_for(2ms);
    }
}

void mark_treadmill() {
    vr::TrackedDevicePose_t devicePose[vr::k_unMaxTrackedDeviceCount];
    vr::VRSystem()->GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin::TrackingUniverseRawAndUncalibrated, 0, devicePose, vr::k_unMaxTrackedDeviceCount);
    ctx.config._POINTC.x = devicePose[0].mDeviceToAbsoluteTracking.m[0][3];
    ctx.config._POINTC.z = devicePose[0].mDeviceToAbsoluteTracking.m[2][3];
    ctx.config._POINTC.ready = true;
}

void MainWindow() {
    if (ImGui::Button(tr(u8"重置位置"))) {
        if (ctx.start) {
            ctx.want_position_reset = true;
        } else {
            ResetPosition();
        }
    }
    if (ImGui::Button(tr(u8"选项"))) {
        active_page = PageT::OPTIONS;
    }
    if (ImGui::Button(ctx.start ? tr(u8"停止") : tr(u8"开始"))) {
        if (ctx.start = !ctx.start) {
            std::thread t(pose_loop);
            t.detach();
        }
    }
    ImGui::Text("%d   (%.2f km)", ctx.walk_record.steps, ctx.walk_record.distance/1000);
}

void OptionsWindow() {
    bool config_changed = false;
    ImGui::SliderInt(tr(u8"移动倍率"), &ctx.config.speed, 0, 200, "%d");
    config_changed |= ImGui::IsItemDeactivatedAfterChange();
    ImGui::SliderFloat(tr(u8"灵敏度"), &ctx.config.limit, 0.f, 0.01f, "%.5f", 2.f);
    config_changed |= ImGui::IsItemDeactivatedAfterChange();
    ImGui::SameLine();
    if (ImGui::Button(tr(u8"评估"))) {
        active_page = PageT::ASSESS_SENSITIVITY;
    }
    ImGui::Text(tr(u8"（如果无法移动请下调该数值）"));
    config_changed |= ImGui::Checkbox(tr(u8"运动平滑"), &ctx.config.smooth);
    config_changed |= ImGui::Checkbox(tr(u8"允许后退"), &ctx.config.back);
    config_changed |= ImGui::Checkbox(tr(u8"跑步机模式"), &ctx.config.treadmill);
    if (ctx.config.treadmill) {
        ImGui::Text(tr(u8"跑步机位置"));
        ImGui::SameLine();
        if (ImGui::Button(tr(u8"标记"))) {
            active_page = PageT::LOCATE_TREADMILL;
            ctx.config._POINTC.ready = false;
        }
    }
    if (ImGui::Button(tr(u8"⇦"))) {
        active_page = PageT::MAIN;
    }
    if (config_changed) {
        SaveConfig();
    }
}

void TrackersWindow() {
    tracker_update();

    const auto make_text_for_tracker = [] (const char* tracker_name, vr::TrackedDeviceIndex_t tracker_id) {
        static const ImVec4 red   = ImVec4(189.f/255.f, 38.f/255.f, 38.f/255.f, 1.f);
        static const ImVec4 green = ImVec4(38.f/255.f, 189.f/255.f, 38.f/255.f, 1.f);
        const bool is_ready = tracker_id != -1;
        ImGui::PushStyleColor(ImGuiCol_Text, is_ready ? green : red);
        ImGui::Text("%s: %s", tracker_name, is_ready ? tr(u8"已就绪") : tr(u8"未就绪"));
        ImGui::PopStyleColor();
    };

    ImGui::TextWrapped(tr(u8"现在需要确认您的追踪器是否已就绪,请确保您已在 SteamVR→设置→控制器→管理追踪器 中设置了相应的位置（腰，左脚，右脚）"));
    make_text_for_tracker(tr(u8"腰"), ctx.tracker.waist);
    make_text_for_tracker(tr(u8"左脚"), ctx.tracker.footL);
    make_text_for_tracker(tr(u8"右脚"), ctx.tracker.footR);

    if (ctx.tracker.ok) {
        if (ImGui::Button(tr(u8"下一步"))) {
            active_page = PageT::MAIN;
        }
    }
}

void SensitivityWindow() {
    if (sensitivity_sample_count == -1) {
        ImGui::TextWrapped(tr(u8"现在需要评估您的追踪器灵敏度, 请保持静止站立, 准备好了请点击下方【开始评估】"));
        if (ImGui::Button(tr(u8"开始评估"))) {
            sensitivity_sample_count = 0;
            std::thread t(assess_tracker_sensitivity);
            t.detach();
        }
    } else if (sensitivity_sample_count == SENSITIVITY_SAMPLE_COUNT_MAX) {
        ImGui::Text(tr(u8"评估完成！"));
        ImGui::Text(tr(u8"灵敏度: %.5f"), ctx.config.limit);
        if (ImGui::Button(tr(u8"完成"))) {
            SaveConfig();
            sensitivity_sample_count = -1;
            active_page = PageT::MAIN;
        }
    } else {
        int percentage = (float) sensitivity_sample_count / SENSITIVITY_SAMPLE_COUNT_MAX * 100;
        ImGui::TextWrapped(tr(u8"评估中, 请保持静止不动。已完成：%d %%"), percentage);
        ImGui::Text(tr(u8"灵敏度: %.5f"), ctx.config.limit);
    }
}

void TreadmillWindow() {
    static ImageTexture treadmill_img;
    if (!treadmill_img.id) {
        treadmill_img = LoadTextureFromFile("assets/4.png");
    }
    ImGui::Image(treadmill_img.id, ImVec2(treadmill_img.width, treadmill_img.height));
    ImGui::SameLine();
    if (ctx.config._POINTC.ready) {
        ImGui::TextWrapped(tr(u8"标记成功，请退回至主页面"));
    } else {
        ImGui::TextWrapped(tr(u8"现在请如左图所示, 站在跑步机后方点击下方【标记】按钮"));
    }
    if (ImGui::Button(tr(u8"标记"))) {
        mark_treadmill();
        SaveConfig();
    }
    if (ctx.config._POINTC.ready) {
        if (ImGui::Button(tr(u8"⇦"))) {
            active_page = PageT::MAIN;
        }  
    }
}

void BuildWindow() {
    auto &io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiSetCond_Always);
	ImGui::SetNextWindowSize(io.DisplaySize, ImGuiSetCond_Always);
    ImGui::Begin( "OpenVR-FBTWalk", nullptr, ImVec2(0.f, 0.f), 1.f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    switch (active_page) {
    case (PageT::MAIN):
        MainWindow();
        break;
    case (PageT::OPTIONS):
        OptionsWindow();
        break;
    case (PageT::DETECT_TRACKERS):
        TrackersWindow();
        break;
    case (PageT::ASSESS_SENSITIVITY):
        SensitivityWindow();
        break;
    case (PageT::LOCATE_TREADMILL):
        TreadmillWindow();
        break;
    }
    ImGui::End();
}
