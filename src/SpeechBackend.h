#pragma once
#include "TextToSpeech.h"
#include <memory>

namespace RayGame::detail {
struct SpeechBackend {
    virtual ~SpeechBackend() = default;
    virtual SpeechState State() = 0;
    virtual std::string Error() = 0;
    virtual bool Speak(const std::u16string& text, const SpeechOptions& options) = 0;
    virtual void Stop() = 0;
};
std::unique_ptr<SpeechBackend> CreateSpeechBackend();
} // namespace RayGame::detail
