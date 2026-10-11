// Optional native-system check: requires an installed voice and audio endpoint.
// All utterances are muted; this verifies synthesis, not audible voice quality.
#include "RayGame.h"
#include <chrono>
#include <cstdio>
#include <thread>

using namespace RayGame;
class SpeechGame : public GameEngine {
public:
    SpeechGame() : GameEngine("speech test", 320, 240) {}
    void Step(bool focus = true) {
        InputFrame f;
        f.focused = focus;
        Input::SetFrame(f);
        Tick(1.0f / 60);
    }
};
int main() {
    int failures = 0;
    auto check = [&](bool ok, const char* name) {
        std::printf("%s %s (%s)\n", ok ? "PASS" : "FAIL", name,
                    TextToSpeech::GetLastError().c_str());
        if (!ok)
            ++failures;
    };
    auto idle = [] {
        auto until = std::chrono::steady_clock::now() + std::chrono::seconds(12);
        // Allow an asynchronous SAPI request to start before checking completion.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        do {
            TextToSpeech::Update();
            if (TextToSpeech::GetState() == SpeechState::Ready)
                return TextToSpeech::GetLastError().empty();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        } while (std::chrono::steady_clock::now() < until);
        return false;
    };
    check(TextToSpeech::Initialize(), "initialize system voice");
    SpeechOptions muted;
    muted.volume = 0;
    muted.rate = 2;
    check(TextToSpeech::Speak(u8"RayGame. 你好 🌍. <speak>literal text</speak>", muted),
          "UTF-8 and literal XML");
    check(idle(), "native synthesis completes");
    muted.language = "zz-ZZ";
    check(!TextToSpeech::Speak("Missing voice", muted), "missing language is reported");
    muted.language.clear();
    check(TextToSpeech::Speak("Recovery", muted) && idle(), "recover after voice selection error");
    check(TextToSpeech::Speak(std::string(2000, 'a'), muted), "maximum length accepted");
    check(TextToSpeech::Speak("Replacement", muted) && idle(), "replace pending speech");
    SpeechGame game;
    for (int operation = 0; operation < 3; ++operation) {
        check(TextToSpeech::Speak(std::string(1000, 'a'), muted), "start interruptible speech");
        if (operation == 0)
            game.Step(false);
        if (operation == 1)
            game.SetPaused(true);
        if (operation == 2) {
            game.LoadScene(CreateRef<Scene>());
            game.Step();
        }
        check(idle(), "focus / pause / scene cancels speech");
        game.SetPaused(false);
    }
    game.Shutdown();
    check(TextToSpeech::GetState() == SpeechState::Uninitialized,
          "headless engine shutdown releases speaker");
    check(TextToSpeech::Initialize() && TextToSpeech::Speak("Reinitialized", muted) && idle(),
          "reinitialize voice");
    TextToSpeech::Shutdown();
    std::printf("%d failures\n", failures);
    return failures ? 1 : 0;
}
