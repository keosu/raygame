#pragma once
#include "World.h"
#include "Profile.h"
#include <raygame/RayGame.h>

namespace Forge {
inline constexpr Color Ink{9,15,23,255}, Steel{20,34,45,255}, Muted{118,145,151,255};
inline constexpr Color Light{224,236,218,255}, Mint{150,245,199,255}, Ember{247,163,82,255}, Red{248,94,79,255};
struct Spark { Vector2 position{}, velocity{}; float age = 0, life = 1, size = 2; Color color{}; };
class Presentation {
public:
    RayGame::FontHandle body, title;
    TextCatalog catalog;
    RayGame::TextureHandle glow;
    Camera2D camera{{640,360},{640,550},0,1};
    std::vector<Spark> particles;
    float clock = 0, shake = 0, flash = 0, landing = 0;
    Settings settings;
    void Init(const std::string& assets);
    void Update(float dt);
    void Follow(const World& world, Vector2 renderPosition, float dt, bool snap = false);
    void Burst(Vector2 position, Color color, int count, float speed);
    void React(unsigned event, Vector2 position);
    void DrawWorld(const World& world, Vector2 runner, bool menu);
    void DrawRunner(Vector2 position, const Runner& state, float scale = 1, float alpha = 1);
    void Text(const std::string& value, Vector2 position, float size, Color color = Light, bool heading = false);
    float Measure(const std::string& value, float size, bool heading = false) const;
    std::string Localize(const std::string& value) const { return catalog.Translate(value,settings.language); }
    RayGame::FontHandle ChineseFont(float size, bool heading = false) const;
    void Glow(Vector2 position, float radius, Color color, float alpha = 0.2f);
private:
    std::uint32_t random = 0x12345678;
    std::string fontDirectory;
    std::vector<int> chineseGlyphs;
    mutable std::map<std::pair<int,bool>,RayGame::FontHandle> chineseFonts;
    RayGame::FontHandle FontFor(const std::string& text, float size, bool heading) const;
    float Random(float min, float max);
    void Background(int theme);
    void DrawPlatform(const Platform& platform, float time, int theme);
};
}
