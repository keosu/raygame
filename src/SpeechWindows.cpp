#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <sapi.h>
#include "SpeechBackend.h"
#include <cmath>
#include <sstream>

namespace RayGame::detail {
namespace {
template <class T> void Release(T*& value) {
    if (value)
        value->Release();
    value = nullptr;
}
class WindowsSpeech final : public SpeechBackend {
    ISpVoice* voice = nullptr;
    ISpObjectToken* defaultVoice = nullptr;
    bool ownsCom = false;
    std::string error;
    bool Check(HRESULT result, const char* action) {
        if (SUCCEEDED(result))
            return true;
        std::ostringstream message;
        message << action << " (HRESULT 0x" << std::hex << static_cast<unsigned long>(result)
                << ")";
        error = message.str();
        return false;
    }

public:
    WindowsSpeech() {
        auto hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        ownsCom = SUCCEEDED(hr);
        if (hr != RPC_E_CHANGED_MODE && !Check(hr, "Cannot initialize COM"))
            return;
        if (!Check(CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_INPROC_SERVER, IID_ISpVoice,
                                    reinterpret_cast<void**>(&voice)),
                   "Cannot create SAPI voice"))
            return;
        if (!Check(voice->GetVoice(&defaultVoice), "No default Windows voice is installed"))
            Release(voice);
    }
    ~WindowsSpeech() override {
        Stop();
        Release(defaultVoice);
        Release(voice);
        if (ownsCom)
            CoUninitialize();
    }
    SpeechState State() override {
        if (!voice)
            return SpeechState::Unavailable;
        // Include queued work before SAPI transitions dwRunningState to speaking.
        auto wait = voice->WaitUntilDone(0);
        if (wait == S_FALSE)
            return SpeechState::Speaking;
        if (!Check(wait, "Cannot query SAPI completion"))
            return SpeechState::Unavailable;
        SPVOICESTATUS status{};
        if (!Check(voice->GetStatus(&status, nullptr), "Cannot query SAPI status"))
            return SpeechState::Unavailable;
        Check(status.hrLastResult, "SAPI synthesis failed");
        return status.dwRunningState == SPRS_IS_SPEAKING ? SpeechState::Speaking
                                                         : SpeechState::Ready;
    }
    std::string Error() override {
        return error;
    }
    bool Speak(const std::u16string& text, const SpeechOptions& options) override {
        error.clear();
        if (!voice) {
            error = "Windows speech is unavailable";
            return false;
        }
        ISpObjectToken* selected = defaultVoice;
        if (selected)
            selected->AddRef();
        if (!options.language.empty()) {
            std::wstring language(options.language.begin(), options.language.end());
            LCID locale = LocaleNameToLCID(language.c_str(), 0);
            ISpObjectTokenCategory* category = nullptr;
            IEnumSpObjectTokens* tokens = nullptr;
            Release(selected);
            if (locale && SUCCEEDED(CoCreateInstance(
                              CLSID_SpObjectTokenCategory, nullptr, CLSCTX_INPROC_SERVER,
                              IID_ISpObjectTokenCategory, reinterpret_cast<void**>(&category)))) {
                std::wostringstream filter;
                filter << L"Language=" << std::hex << LANGIDFROMLCID(locale);
                if (SUCCEEDED(category->SetId(SPCAT_VOICES, FALSE)) &&
                    SUCCEEDED(category->EnumTokens(filter.str().c_str(), nullptr, &tokens)))
                    tokens->Next(1, &selected, nullptr);
            }
            Release(tokens);
            Release(category);
            if (!selected) {
                error = "No installed Windows SAPI voice for " + options.language;
                return false;
            }
        }
        auto hr = voice->SetVoice(selected);
        Release(selected);
        if (!Check(hr, "Cannot select SAPI voice") ||
            !Check(voice->SetRate(static_cast<long>(std::lround(std::log2(options.rate) * 5))),
                   "Cannot set speech rate") ||
            !Check(voice->SetVolume(static_cast<USHORT>(std::lround(options.volume * 100))),
                   "Cannot set speech volume"))
            return false;
        static_assert(sizeof(wchar_t) == sizeof(char16_t));
        return Check(voice->Speak(reinterpret_cast<const wchar_t*>(text.c_str()),
                                  SPF_ASYNC | SPF_PURGEBEFORESPEAK | SPF_IS_NOT_XML, nullptr),
                     "Cannot speak text");
    }
    void Stop() override {
        if (voice)
            voice->Speak(nullptr, SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr);
    }
};
} // namespace
std::unique_ptr<SpeechBackend> CreateSpeechBackend() {
    return std::make_unique<WindowsSpeech>();
}
} // namespace RayGame::detail
#endif
