#pragma once
#include "ExampleUI.h"
#include <map>
#include <sstream>

namespace Campaign {
class TextUI {
public:
    static inline bool chinese = true;
    static inline std::map<std::string, std::string> catalog;
    static std::string Tr(const std::string& text) {
        auto found = catalog.find(text);
        return chinese && found != catalog.end() ? found->second : text;
    }
    static void Load(const std::string& path, const std::string& fonts, const std::string& id) {
        std::string content;
        if (!RayGame::FileSystem::ReadAssetText(path, content))
            throw std::runtime_error("Cannot load campaign translations: " + path);
        catalog.clear();
        std::istringstream input(content);
        std::string line, characters;
        while (std::getline(input, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            auto tab = line.find('\t');
            if (tab == std::string::npos || line.empty() || line[0] == '#')
                continue;
            catalog[line.substr(0, tab)] = line.substr(tab + 1);
            characters += line.substr(tab + 1);
        }
        std::vector<int> glyphs;
        for (int c = 32; c < 127; ++c)
            glyphs.push_back(c);
        int count = 0;
        auto* points = LoadCodepoints(characters.c_str(), &count);
        if (points) {
            glyphs.insert(glyphs.end(), points, points + count);
            UnloadCodepoints(points);
        }
        std::sort(glyphs.begin(), glyphs.end());
        glyphs.erase(std::unique(glyphs.begin(), glyphs.end()), glyphs.end());
        auto body = RayGame::ResourceManager::LoadFontResource(
            id + "-sc-body", fonts + "CampaignSC-Medium.ttf", 96, glyphs);
        auto heading = RayGame::ResourceManager::LoadFontResource(
            id + "-sc-title", fonts + "CampaignSC-SemiBold.ttf", 128, glyphs);
        if (!body || !heading)
            throw std::runtime_error("Campaign Chinese fonts are missing");
        SetTextureFilter(body->texture, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(heading->texture, TEXTURE_FILTER_BILINEAR);
        RayGame::UI::unicodeFont = body;
        RayGame::UI::unicodeHeadingFont = heading;
        RayGame::UI::unicodeScale = 1.4f; // Noto SC's ascent box is taller than its CJK em square.
    }
    static void Label(float x, float y, const std::string& text, int size = 20,
                      Color color = RAYWHITE) {
        RayGame::UI::Label(x, y, Tr(text), size, color);
    }
    static void LabelCentered(float x, float y, const std::string& text, int size = 20,
                              Color color = RAYWHITE) {
        RayGame::UI::LabelCentered(x, y, Tr(text), size, color);
    }
    static void Title(float x, float y, const std::string& text, int size = 36,
                      Color color = RAYWHITE) {
        RayGame::UI::Title(x, y, Tr(text), size, color);
    }
    static void TitleCentered(float x, float y, const std::string& text, int size = 36,
                              Color color = RAYWHITE) {
        RayGame::UI::TitleCentered(x, y, Tr(text), size, color);
    }
};
} // namespace Campaign
