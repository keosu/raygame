#pragma once
#include <limits>

static void TestSpeech() {
    printf("[speech input contract]\n");
    TextToSpeech::Shutdown();
    for (const auto& text :
         {std::string(), std::string("a\0b", 3), std::string("\xc0\xaf"),
          std::string("\xed\xa0\x80"), std::string("\xf4\x90\x80\x80"), std::string("\xe4\xb8"),
          std::string("\x80"), std::string(2001, 'a')}) {
        CHECK(!TextToSpeech::Speak(text));
        CHECK(TextToSpeech::GetState() == SpeechState::Uninitialized);
    }
    SpeechOptions options;
    for (float value : {-1.0f, 0.0f, 2.1f, std::numeric_limits<float>::quiet_NaN()}) {
        options.rate = value;
        CHECK(!TextToSpeech::Speak("test", options));
    }
    options = {};
    options.volume = std::numeric_limits<float>::infinity();
    CHECK(!TextToSpeech::Speak("test", options));
    options = {};
    options.language = "en-US;Gender=Male";
    CHECK(!TextToSpeech::Speak("test", options));
    CHECK(!TextToSpeech::GetLastError().empty());
    TextToSpeech::Stop();
    TextToSpeech::Shutdown();
    CHECK(TextToSpeech::GetLastError().empty());
#if !defined(_WIN32) && !defined(__ANDROID__)
    CHECK(!TextToSpeech::IsSupported());
    CHECK(!TextToSpeech::Speak(u8"你好 🌍"));
    CHECK(TextToSpeech::GetState() == SpeechState::Unavailable);
    TextToSpeech::Shutdown();
#endif
}
