#pragma once

#include "Core.h"
#include <fstream>
#include <sstream>

namespace RayGame {

// Minimal dependency-free key/value save store. Persists as `key=value` lines.
class RAYGAME_API SaveSystem {
public:
    void SetString(const std::string& key, const std::string& value) { data[key] = value; }
    void SetInt(const std::string& key, int value) { data[key] = std::to_string(value); }
    void SetFloat(const std::string& key, float value) { data[key] = std::to_string(value); }
    void SetBool(const std::string& key, bool value) { data[key] = value ? "1" : "0"; }

    std::string GetString(const std::string& key, const std::string& def = "") const {
        auto it = data.find(key);
        return it != data.end() ? it->second : def;
    }
    int GetInt(const std::string& key, int def = 0) const {
        auto it = data.find(key);
        if (it == data.end()) return def;
        try {
            size_t used = 0;
            int value = std::stoi(it->second, &used);
            return used == it->second.size() ? value : def;
        } catch (...) { return def; }
    }
    float GetFloat(const std::string& key, float def = 0.0f) const {
        auto it = data.find(key);
        if (it == data.end()) return def;
        try {
            size_t used = 0;
            float value = std::stof(it->second, &used);
            return used == it->second.size() && std::isfinite(value) ? value : def;
        } catch (...) { return def; }
    }
    bool GetBool(const std::string& key, bool def = false) const {
        auto it = data.find(key);
        if (it == data.end()) return def;
        if (it->second == "1" || it->second == "true") return true;
        if (it->second == "0" || it->second == "false") return false;
        return def;
    }

    bool Has(const std::string& key) const { return data.count(key) > 0; }
    void Remove(const std::string& key) { data.erase(key); }
    void ClearAll() { data.clear(); }

    bool Save(const std::string& filepath) const {
        std::ofstream out(filepath, std::ios::trunc);
        if (!out.is_open()) return false;
        for (const auto& kv : data) out << Escape(kv.first) << "=" << Escape(kv.second) << "\n";
        out.flush();
        return out.good();
    }

    bool Load(const std::string& filepath) {
        std::ifstream in(filepath);
        if (!in.is_open()) return false;
        std::unordered_map<std::string, std::string> loaded;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;
            size_t eq = 0;
            for (; eq < line.size(); ++eq) {
                if (line[eq] == '\\') { if (eq + 1 < line.size()) ++eq; }
                else if (line[eq] == '=') break;
            }
            if (eq == line.size()) continue;
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            if (!value.empty() && value.back() == '\r') value.pop_back();
            loaded[Unescape(key)] = Unescape(value);
        }
        if (in.bad()) return false;
        data.swap(loaded);
        return true;
    }

private:
    static std::string Escape(const std::string& value) {
        std::string out;
        for (char c : value) {
            if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else {
                if (c == '\\' || c == '=' || c == '#') out += '\\';
                out += c;
            }
        }
        return out;
    }

    static std::string Unescape(const std::string& value) {
        std::string out;
        for (size_t i = 0; i < value.size(); ++i) {
            if (value[i] == '\\' && i + 1 < value.size()) {
                char c = value[i + 1];
                if (c == 'n') { out += '\n'; ++i; continue; }
                if (c == 'r') { out += '\r'; ++i; continue; }
                if (c == '\\' || c == '=' || c == '#') { out += c; ++i; continue; }
            }
            out += value[i];
        }
        return out;
    }

    std::unordered_map<std::string, std::string> data;
};

}
