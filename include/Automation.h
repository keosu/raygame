#pragma once

#include "Core.h"
#include "Input.h"
#include <filesystem>
#include <functional>

namespace RayGame {

// Scripted runs for screenshot tours and smoke checks. A run is a timeline of actions keyed by
// frame number. Actions can load scenes or call game code, inject input that reaches the game
// through Input exactly like a player's, and save screenshots. GameEngine begins each frame of
// the run before capturing input; injected input is layered over the devices and the window
// always reads as focused, so a run works in a background or hidden window.
class RAYGAME_API Automation {
public:
    struct Options {
        bool hiddenWindow = false; // GameEngine::Init creates the window hidden (desktop only)
    };

    // Starts a run writing into `directory` (created if needed). Returns false, leaving no run
    // active, if the directory cannot be created. Call before GameEngine::Init for hiddenWindow.
    static bool Start(const std::filesystem::path& directory, Options options = {});
    static void Stop();
    static bool Active() { return active; }
    static bool HiddenWindow() { return active && options.hiddenWindow; }
    static const std::filesystem::path& Directory() { return directory; }
    static int Frame() { return frame; } // frames begun since Start; the first one is 1

    // Runs `action` when frame `frame` begins, after Every actions, in the order added.
    static void At(int frame, std::function<void()> action);
    // Runs `action` at the start of every frame with the frame number.
    static void Every(std::function<void(int frame)> action);

    // Input for the current frame, in canvas coordinates. A click presses this frame and
    // releases on the next; a key press likewise; a tap is one frame of touch contact.
    static void Click(Vector2 canvas, int button = MOUSE_BUTTON_LEFT);
    static void PressKey(int key);
    static void Tap(Vector2 canvas, int touchId = 0);

    // The last rendered canvas, opaque like the window, at its logical size unless `size` is
    // given. Names without an extension get ".png"; relative paths go under Directory().
    static bool SaveCanvas(const std::string& name, Vector2 size = {0, 0});
    // The last frame as presented in the window, margins included.
    static bool SaveWindow(const std::string& name);

    // Called by GameEngine at the start of each frame, before input is captured.
    static void BeginFrame();

private:
    struct Timed {
        int frame;
        std::function<void()> action;
    };
    struct Press {
        Vector2 canvas;
        int code;
    };
    static void Overlay(InputFrame& input);
    static std::filesystem::path OutputPath(const std::string& name);

    static inline bool active = false;
    static inline Options options;
    static inline std::filesystem::path directory;
    static inline int frame = 0;
    static inline std::vector<Timed> timeline;
    static inline std::vector<std::function<void(int)>> everyFrame;
    static inline std::vector<Press> clicks, releases, taps;
    static inline std::vector<int> keys, keyReleases;
};

} // namespace RayGame
