#pragma once

#include "Controls.h"

namespace UQ {

// The transformation cut-in: three medals into the Z Riser, then Ultraman Z. Its timing
// matches assets/audio/transform.wav (medals at 0.25/0.7/1.15 s, burst at 2.6 s).
class TransformScene : public Scene {
public:
    void OnLoad() override;
    void Update() override;
    void Render() override;
    float Time() const { return time; }

private:
    Controls controls;
    float time = 0;
    bool left = false;
};

class TitleScene : public Scene {
public:
    void OnLoad() override;
    void Update() override;
    void Render() override;
    enum class Page { Main, Help, Settings };
    void SetPage(Page p) { page = p; pageTime = 0; }

private:
    Controls controls;
    Page page = Page::Main;
    float time = 0;
    float pageTime = 0;
    void DrawBackdrop() const;
    void DrawMain();
    void DrawHelp();
    void DrawSettings();
};

} // namespace UQ
