#pragma once

#include "Game.h"
#include "Map.h"

namespace UG {

class MenuScene : public Scene {
public:
    explicit MenuScene(MenuPage page);
    void OnLoad() override;
    void OnUnload() override;
    void Update() override;
    void Render() override;

private:
    MenuPage page;
    float pageTime = 0;
    int heroIndex = 0;
    int kaijuIndex = 0;
    static constexpr int kLevelsPerPage = 6;
    int levelPage = 0;
    bool codex = false;
    int codexIndex = 0;
    RenderTexture2D sky{};
    std::vector<RenderTexture2D> previews;
    std::vector<MapData> maps;

    struct Meteor {
        Vector2 p, v;
        float life;
    };
    std::vector<Meteor> meteors;

    void SetPage(MenuPage next);
    void DrawBackground();
    void DrawTitle();
    void DrawLevels();
    void DrawHeroes();
    void DrawCodex();
    void DrawKaiju();
    void DrawHelp();
    void DrawBack();
    void DrawPageTitle(const char* title, const char* sub);
};

void PreviewCodex();

} // namespace UG
