#include "TextToSpeech.h"
#include "SpeechBackend.h"
#include <cmath>
#include <optional>

namespace RayGame {
namespace {
struct Request {
    std::u16string text;
    SpeechOptions options;
};
std::unique_ptr<detail::SpeechBackend> backend;
std::optional<Request> pending;
SpeechState state = SpeechState::Uninitialized;
std::string error;

// JNI NewString takes UTF-16, not UTF-8 or JNI's modified UTF-8. Validate once for both backends.
bool Decode(const std::string& text, std::u16string& result) {
    if (text.empty() || text.size() > 8000)
        return false;
    for (size_t i = 0; i < text.size();) {
        unsigned c = static_cast<unsigned char>(text[i++]);
        unsigned count = 0, minimum = 0;
        if (c >= 0xc2 && c <= 0xdf) {
            count = 1;
            minimum = 0x80;
            c &= 0x1f;
        } else if (c >= 0xe0 && c <= 0xef) {
            count = 2;
            minimum = 0x800;
            c &= 0x0f;
        } else if (c >= 0xf0 && c <= 0xf4) {
            count = 3;
            minimum = 0x10000;
            c &= 7;
        } else if (c >= 0x80)
            return false;
        if (i + count > text.size())
            return false;
        for (unsigned n = 0; n < count; ++n) {
            unsigned next = static_cast<unsigned char>(text[i++]);
            if ((next & 0xc0) != 0x80)
                return false;
            c = (c << 6) | (next & 0x3f);
        }
        if (c < minimum || c == 0 || c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff))
            return false;
        if (c >= 0x10000) {
            c -= 0x10000;
            result.push_back(static_cast<char16_t>(0xd800 + (c >> 10)));
            result.push_back(static_cast<char16_t>(0xdc00 + (c & 1023)));
        } else
            result.push_back(static_cast<char16_t>(c));
        if (result.size() > 2000)
            return false;
    }
    return true;
}
bool ValidOptions(const SpeechOptions& options) {
    if (!std::isfinite(options.rate) || options.rate < 0.5f || options.rate > 2 ||
        !std::isfinite(options.volume) || options.volume < 0 || options.volume > 1 ||
        options.language.size() > 63)
        return false;
    for (char c : options.language)
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
              c == '-'))
            return false;
    return true;
}
} // namespace

bool TextToSpeech::IsSupported() {
#if defined(_WIN32) || defined(__ANDROID__)
    return true;
#else
    return false;
#endif
}
bool TextToSpeech::Initialize() {
    if (backend)
        return state != SpeechState::Unavailable;
    error.clear();
    backend = detail::CreateSpeechBackend();
    if (!backend) {
        state = SpeechState::Unavailable;
        error = "System speech is supported on Windows and Android only";
        return false;
    }
    Update();
    return state != SpeechState::Unavailable;
}
bool TextToSpeech::Speak(const std::string& utf8, const SpeechOptions& options) {
    Request request;
    if (!ValidOptions(options) || !Decode(utf8, request.text)) {
        error = "Speech requires valid UTF-8 (1..2000 UTF-16 units), a language tag, rate 0.5..2 "
                "and volume 0..1";
        return false;
    }
    if (!Initialize())
        return false;
    request.options = options;
    pending = std::move(request);
    error.clear();
    Update();
    return error.empty();
}
void TextToSpeech::Update() {
    if (!backend)
        return;
    state = backend->State();
    auto backendError = backend->Error();
    if (!backendError.empty())
        error = std::move(backendError);
    if (state == SpeechState::Unavailable)
        pending.reset();
    if (pending && (state == SpeechState::Ready || state == SpeechState::Speaking)) {
        auto request = std::move(*pending);
        pending.reset();
        if (!backend->Speak(request.text, request.options))
            error = backend->Error();
        else
            error.clear();
        state = backend->State();
    }
}
void TextToSpeech::Stop() {
    pending.reset();
    if (backend) {
        backend->Stop();
        state = backend->State();
    }
}
void TextToSpeech::Shutdown() {
    pending.reset();
    backend.reset();
    state = SpeechState::Uninitialized;
    error.clear();
}
SpeechState TextToSpeech::GetState() {
    return state;
}
std::string TextToSpeech::GetLastError() {
    return error;
}

#if !defined(_WIN32) && !defined(__ANDROID__)
std::unique_ptr<detail::SpeechBackend> detail::CreateSpeechBackend() {
    return {};
}
#endif
} // namespace RayGame
