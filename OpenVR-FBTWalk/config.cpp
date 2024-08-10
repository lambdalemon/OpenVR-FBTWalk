#include "config.h"

#include "fbtwalk_context.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

constexpr char CONFIG_PATH[] = "config.json";
using json = nlohmann::json;

void LoadConfig() {
    std::ifstream f(CONFIG_PATH);
    if (!f) {
        std::cerr << CONFIG_PATH << " not found, using default values.\n";
        return;
    }
    json obj = json::parse(f);

    ctx.config.treadmill = obj["treadmill"].get<bool>();
    ctx.config.speed = obj["speed"].get<int>();
    ctx.config.limit = obj["limit"].get<float>();
    ctx.config._POINTC.x = obj["POINTC_x"].get<float>();
    ctx.config._POINTC.z = obj["POINTC_z"].get<float>();
    ctx.config._POINTC.ready = obj["POINTC_ready"].get<bool>();
    ctx.config.smooth = obj["smooth"].get<bool>();
    ctx.config.back = obj["back"].get<bool>();
    std::cout << "Loaded config from " << CONFIG_PATH << std::endl;
}

void SaveConfig() {
    json obj;
    obj["treadmill"] = ctx.config.treadmill;
    obj["speed"] = ctx.config.speed;
    obj["limit"] = ctx.config.limit;
    obj["POINTC_x"] = ctx.config._POINTC.x;
    obj["POINTC_z"] = ctx.config._POINTC.z;
    obj["POINTC_ready"] = ctx.config._POINTC.ready;
    obj["smooth"] = ctx.config.smooth;
    obj["back"] = ctx.config.back;

    std::ofstream f(CONFIG_PATH);
    f << obj.dump(4);
    std::cout << "Saved config to " << CONFIG_PATH << std::endl;
}
