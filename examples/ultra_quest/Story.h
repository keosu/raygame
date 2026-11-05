#pragma once

#include "Common.h"
#include <map>

namespace UQ {

// The chapter script, assets/story/chapter1.txt:
//   :: key | display name | #rrggbb       a speaker
//   == block                               starts a block of lines
//   key | text                             one line; "narrator" has no portrait
struct Speaker {
    std::string name;
    Color color = WHITE;
};

struct Line {
    std::string speaker;
    std::string text;
};

namespace Story {
void Load();
const Speaker& SpeakerFor(const std::string& key);
const std::vector<Line>& Block(const std::string& id); // empty for unknown ids
} // namespace Story

// A dialogue box with a typewriter. Feed it Confirm presses; it reports when the block ends.
class Dialogue {
public:
    void Start(const std::string& block);
    bool Active() const { return active; }
    const std::string& Current() const { return block; }
    // Returns true on the frame the last line is dismissed.
    bool Update(float dt, bool confirm);
    void Draw() const;
    void Skip() { active = false; }

private:
    std::string block;
    std::vector<Line> lines;
    int index = 0;
    float shown = 0; // characters revealed
    float age = 0;
    bool active = false;
    int blips = 0;
    int Length() const;
};

int Utf8Length(const std::string& s);
std::string Utf8Prefix(const std::string& s, int count);

} // namespace UQ
