#pragma once

#include "Game.h"
#include "Map.h"

namespace UG {

class BattleScene;

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
    // Skill effects on the database page: a demo battle the menu ticks, drawn without field or kaiju.
    Ref<BattleScene> preview;
    int previewHero = -1;
    RenderTexture2D sky{};
    std::vector<RenderTexture2D> previews;
    std::vector<MapData> maps;
    RenderTexture2D survivalPreview{};
    MapData survivalMap;
    int prepareLevel = 0; // level index on the preparation page, or kSurvivalIndex
    std::vector<HeroId> picks;
    float pickWarning = 0; // "lineup full" hint timer

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
    void SyncPreview(float dt);
    void DropPreview();
    void DrawKaiju();
    void DrawHelp();
    void DrawPrepare();
    void OpenPrepare(int level);
    const LevelDef* PreparedLevel() const;
    bool LineupReady() const;
    void Launch();
    void DrawBack();
    void DrawPageTitle(const char* title, const char* sub);
};

// Opens the database page on this hero the next time a MenuScene starts there (capture tour).
void PreviewHero(int hero);

} // namespace UG
