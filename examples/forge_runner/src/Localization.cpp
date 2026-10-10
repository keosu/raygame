#include "Localization.h"
#include <raygame/FileSystem.h>
#include <set>
#include <sstream>

namespace Forge {
const char* LanguageCode(Language language) {
    return language == Language::English ? "en" : "zh-CN";
}
Language ParseLanguage(const std::string& code) {
    return code == "en" ? Language::English : Language::Chinese;
}
void TextCatalog::Load(const std::string& path) {
    std::string text;
    if (!RayGame::FileSystem::ReadAssetText(path, text, 128 * 1024))
        throw std::runtime_error("Cannot read translations: " + path);
    std::map<std::string, std::string> loaded;
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty() || line[0] == '#')
            continue;
        auto separator = line.find('\t');
        if (separator == std::string::npos || separator == 0 || separator == line.size() - 1 ||
            line.find('\t', separator + 1) != std::string::npos)
            throw std::runtime_error("Invalid translation row: " + path);
        if (!loaded.emplace(line.substr(0, separator), line.substr(separator + 1)).second)
            throw std::runtime_error("Duplicate translation: " + line.substr(0, separator));
    }
    if (loaded.empty())
        throw std::runtime_error("Empty translation catalog: " + path);
    translations = std::move(loaded);
}
const std::string& TextCatalog::Translate(const std::string& english, Language language) const {
    if (language == Language::Chinese) {
        auto found = translations.find(english);
        if (found != translations.end())
            return found->second;
    }
    return english; // Custom level text remains readable until it gets a translation.
}
std::vector<int> TextCatalog::ChineseCodepoints() const {
    std::set<int> points;
    for (int code = 32; code < 127; ++code)
        points.insert(code);
    for (const auto& entry : translations) {
        int count = 0;
        int* codes = LoadCodepoints(entry.second.c_str(), &count);
        if (!codes)
            throw std::runtime_error("Cannot decode translation glyphs");
        for (int i = 0; i < count; ++i)
            points.insert(codes[i]);
        UnloadCodepoints(codes);
    }
    return {points.begin(), points.end()};
}
} // namespace Forge
