#pragma once

#include "Export.h"
#include <string>

namespace RayGame {

enum class SpeechState { Uninitialized, Initializing, Ready, Speaking, Unavailable };

struct SpeechOptions {
    std::string language; // BCP-47, e.g. en-US or zh-CN; empty uses the system default.
    float rate = 1;       // 0.5 .. 2, relative to the engine's normal speech rate.
    float volume = 1;     // 0 .. 1, independent of game audio.
};

// One application-wide speaker. Call from the game/runtime thread.
// Speak replaces the current/pending utterance; true means accepted, not completed.
// GameEngine pumps Update and stops speech on focus loss, scene changes and shutdown.
class RAYGAME_API TextToSpeech {
public:
    static bool IsSupported();
    static bool Initialize();
    static bool Speak(const std::string& utf8, const SpeechOptions& options = {});
    static void Stop();
    static void Shutdown();
    static void Update(); // Also call periodically when used without GameEngine.
    static SpeechState GetState();
    static std::string GetLastError();
};

} // namespace RayGame
