#include "Story.h"
#include "Assets.h"
#include "Characters.h"
#include "Draw.h"
#include "Gui.h"
#include <sstream>

namespace UQ {

namespace {
std::map<std::string, Speaker> speakers;
std::map<std::string, std::vector<Line>> blocks;

std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r"), b = s.find_last_not_of(" \t\r");
    return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}

Color ParseColor(const std::string& hex) {
    unsigned v = 0xffffff;
    if (hex.size() == 7 && hex[0] == '#') v = (unsigned)std::stoul(hex.substr(1), nullptr, 16);
    return {(unsigned char)(v >> 16), (unsigned char)(v >> 8), (unsigned char)v, 255};
}

std::vector<std::string> Split(const std::string& s) {
    std::vector<std::string> parts;
    std::stringstream in(s);
    for (std::string part; std::getline(in, part, '|');) parts.push_back(Trim(part));
    return parts;
}

size_t CharLength(unsigned char c) { return c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : 4; }
} // namespace

int Utf8Length(const std::string& s) {
    int n = 0;
    for (size_t i = 0; i < s.size(); i += CharLength((unsigned char)s[i])) ++n;
    return n;
}

std::string Utf8Prefix(const std::string& s, int count) {
    size_t i = 0;
    for (int n = 0; n < count && i < s.size(); ++n) i += CharLength((unsigned char)s[i]);
    return s.substr(0, std::min(i, s.size()));
}

namespace Story {
void Load() {
    speakers.clear();
    blocks.clear();
    std::string text;
    FileSystem::ReadAssetText(AssetPath("story/chapter1.txt"), text);
    std::istringstream lines(text);
    std::string current;
    for (std::string raw; std::getline(lines, raw);) {
        std::string line = Trim(raw);
        if (line.empty() || line[0] == '#') continue;
        if (line.rfind("::", 0) == 0) {
            auto parts = Split(line.substr(2));
            if (parts.size() >= 3) speakers[parts[0]] = {parts[1], ParseColor(parts[2])};
        } else if (line.rfind("==", 0) == 0) {
            current = Trim(line.substr(2));
            blocks[current];
        } else if (!current.empty()) {
            size_t bar = line.find('|');
            if (bar == std::string::npos) continue;
            blocks[current].push_back({Trim(line.substr(0, bar)), Trim(line.substr(bar + 1))});
        }
    }
}

const Speaker& SpeakerFor(const std::string& key) {
    static const Speaker none;
    auto it = speakers.find(key);
    return it != speakers.end() ? it->second : none;
}

const std::vector<Line>& Block(const std::string& id) {
    static const std::vector<Line> empty;
    auto it = blocks.find(id);
    return it != blocks.end() ? it->second : empty;
}
} // namespace Story

// ---------------------------------------------------------------- dialogue

void Dialogue::Start(const std::string& id) {
    block = id;
    lines = Story::Block(id);
    index = 0;
    shown = 0;
    age = 0;
    blips = 0;
    active = !lines.empty();
}

int Dialogue::Length() const { return index < (int)lines.size() ? Utf8Length(lines[index].text) : 0; }

bool Dialogue::Update(float dt, bool confirm) {
    if (!active) return false;
    age += dt;
    int length = Length();
    if (shown < length) {
        shown = std::min((float)length, shown + dt * 34);
        if ((int)shown / 3 > blips) {
            blips = (int)shown / 3;
            Sfx::Play("talk", 0.35f, 0.1f);
        }
    }
    if (!confirm || age < 0.15f) return false;
    if (shown < length) {
        shown = (float)length;
        return false;
    }
    ++index;
    shown = 0;
    blips = 0;
    age = 0;
    if (index >= (int)lines.size()) {
        active = false;
        return true;
    }
    return false;
}

void Dialogue::Draw() const {
    if (!active || index >= (int)lines.size()) return;
    const Line& line = lines[index];
    bool narrator = line.speaker == "narrator";
    float rise = EaseOut(age / 0.18f);
    Rectangle box = {150, 512 + (1 - rise) * 30, 980, 176};
    Draw::RoundedPanel({box.x + 3, box.y + 6, box.width, box.height}, 14, WithAlpha(BLACK, 0.4f), BLANK, 0);
    Draw::RoundedPanel(box, 14, {12, 18, 38, 236}, WithAlpha(Palette::cyan, 0.5f), 2);
    float textX = box.x + 34;
    if (!narrator) {
        Rectangle portrait = {box.x + 22, box.y - 46, 150, 150};
        DrawPortrait(line.speaker, portrait);
        textX = box.x + 196;
        const Speaker& who = Story::SpeakerFor(line.speaker);
        float w = Gui::Measure(who.name, 22, true) + 34;
        Draw::RoundedPanel({textX - 6, box.y - 22, w, 38}, 10, who.color, BLANK, 0);
        Gui::Text(who.name, textX + 11, box.y - 14, 22, Palette::ink, Gui::Align::Left, true);
    }
    std::string visible = Utf8Prefix(line.text, (int)shown);
    Gui::Paragraph(visible, textX, box.y + 34, box.x + box.width - 40 - textX, 25,
                   narrator ? Lighten(Palette::muted, 0.3f) : Palette::text, 1.5f);
    if (shown >= Length()) {
        float bounce = std::sin(age * 7) * 3;
        Draw::Tri({box.x + box.width - 44, box.y + box.height - 34 + bounce},
                  {box.x + box.width - 24, box.y + box.height - 34 + bounce},
                  {box.x + box.width - 34, box.y + box.height - 22 + bounce}, Palette::gold);
    }
}

} // namespace UQ
