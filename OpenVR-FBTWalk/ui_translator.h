#pragma once

#include <unordered_map>
#include <string>
#include <sstream>
#include <locale>

struct UITranslator {
    const std::unordered_map<std::string, std::string> _data;

    const char* operator() (const char* s) const {
        const auto it = _data.find(std::string(s));
        return it == _data.end() ? s : it->second.c_str();
    }
    
    static const UITranslator ZH_HANS;
    static const UITranslator ZH_HANT;
    static const UITranslator JA;
    static const UITranslator KO;
    static const UITranslator EN;

    static const UITranslator& from_locale() {
        const std::string locale_name = std::locale("").name();
        if (locale_name.find("zh-CN") != -1 || locale_name.find("zh-SG") != -1 || locale_name.find("zh-MY") != -1) {
            return ZH_HANS;
        } else if (locale_name.find("zh-") != -1) {
            return ZH_HANT;
        } else if (locale_name.find("ja-") != -1) {
            return JA;
        } else if (locale_name.find("ko-") != -1) {
            return KO;
        } else {
            return EN;
        }
    }

    std::string all_glyphs() const {
        std::stringstream ss;
        for (const auto& [k,v] : _data) {
            ss << v;
        }
        return ss.str();
    }
};
