#pragma once

#include "Core.h"
#include "FileSystem.h"
#include "Diagnostics.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdint>

namespace RayGame {

// Versioned key/value saves with an integrity checksum and atomic replacement.
// Legacy key=value files remain readable. The checksum detects corruption, not tampering.
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

    bool Save(const std::filesystem::path& filepath) const {
        std::vector<std::string> keys;
        for (const auto& kv : data) keys.push_back(kv.first);
        std::sort(keys.begin(), keys.end());
        std::ostringstream body;
        for (const auto& key : keys) body << Escape(key) << "=" << Escape(data.at(key)) << "\n";
        std::string content = body.str();
        if (content.size() > 1024 * 1024 - 64) return false;
        std::ostringstream header;
        header << "# RayGame save 1 " << std::hex << Checksum(content) << "\n";
        // Preserve an existing good backup if the main file is already damaged.
        SaveSystem previous;
        bool validPrevious = previous.LoadFile(filepath);
        bool ok = FileSystem::WriteAtomic(filepath, header.str() + content, validPrevious);
        if (!ok) Diagnostics::Write(LogLevel::Error, "Cannot save: " + filepath.u8string());
        return ok;
    }

    bool Load(const std::filesystem::path& filepath) {
        recovered = false;
        if (LoadFile(filepath)) return true;
        auto backup = filepath; backup += ".bak";
        if (!LoadFile(backup)) return false;
        recovered = true;
        Diagnostics::Write(LogLevel::Warning, "Recovered save from backup: " + backup.u8string());
        return true;
    }

    bool WasRecovered() const { return recovered; }

private:
    bool recovered = false;

    static std::uint64_t Checksum(const std::string& value) {
        std::uint64_t hash = 14695981039346656037ull;
        for (unsigned char c : value) { hash ^= c; hash *= 1099511628211ull; }
        return hash;
    }

    bool LoadFile(const std::filesystem::path& filepath) {
        std::string content;
        if (!FileSystem::ReadText(filepath, content)) return false;
        bool versioned = false;
        if (content.compare(0, 15, "# RayGame save ") == 0) {
            versioned = true;
            size_t lineEnd = content.find('\n');
            if (lineEnd == std::string::npos) return false;
            constexpr const char* prefix = "# RayGame save 1 ";
            if (content.compare(0, 17, prefix) != 0) return false;
            std::string checksum = content.substr(17, lineEnd - 17);
            content.erase(0, lineEnd + 1);
            try {
                size_t used = 0;
                auto expected = std::stoull(checksum, &used, 16);
                if (used != checksum.size() || expected != Checksum(content)) return false;
            } catch (...) { return false; }
        }
        std::istringstream in(content);
        std::unordered_map<std::string, std::string> loaded;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;
            size_t eq = 0;
            for (; eq < line.size(); ++eq) {
                if (line[eq] == '\\') { if (eq + 1 < line.size()) ++eq; }
                else if (line[eq] == '=') break;
            }
            if (eq == line.size()) return false;
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            if (!value.empty() && value.back() == '\r') value.pop_back();
            loaded[Unescape(key)] = Unescape(value);
        }
        if (in.bad()) return false;
        if (!versioned && loaded.empty()) return false;
        data.swap(loaded);
        return true;
    }

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
