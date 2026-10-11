#if defined(__ANDROID__)
#include "SpeechBackend.h"
#include "AndroidPlatform.h"
#include <jni.h>

namespace RayGame::detail {
namespace {
struct JavaFrame {
    JavaVM* vm = nullptr;
    JNIEnv* env = nullptr;
    bool attached = false, frame = false;
    explicit JavaFrame(JavaVM* value) : vm(value) {
        if (!vm)
            return;
        if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_EDETACHED) {
            if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
                env = nullptr;
                return;
            }
            attached = true;
        }
        if (env && env->PushLocalFrame(24) == JNI_OK)
            frame = true;
        else {
            if (env && env->ExceptionCheck())
                env->ExceptionClear();
            env = nullptr;
        }
    }
    ~JavaFrame() {
        if (frame)
            env->PopLocalFrame(nullptr);
        if (attached)
            vm->DetachCurrentThread();
    }
    bool Good() {
        if (!env)
            return false;
        if (!env->ExceptionCheck())
            return true;
        env->ExceptionClear();
        return false;
    }
};
class AndroidSpeech final : public SpeechBackend {
    JavaVM* vm = nullptr;
    jobject bridge = nullptr;
    jmethodID stateMethod = nullptr, errorMethod = nullptr, speakMethod = nullptr;
    jmethodID stopMethod = nullptr, closeMethod = nullptr;
    std::string error;
    bool Check(JavaFrame& java) {
        if (java.Good())
            return true;
        error = "Android TTS JNI call failed";
        return false;
    }

public:
    AndroidSpeech() {
        auto* activity = AndroidActivity();
        if (!activity) {
            error = "Android NativeActivity is unavailable";
            return;
        }
        vm = activity->vm;
        JavaFrame java(vm);
        if (!Check(java))
            return;
        auto* env = java.env;
        auto activityClass = env->GetObjectClass(activity->clazz);
        if (!Check(java) || !activityClass)
            return;
        auto getLoader =
            env->GetMethodID(activityClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
        if (!Check(java) || !getLoader)
            return;
        auto loader = env->CallObjectMethod(activity->clazz, getLoader);
        if (!Check(java) || !loader)
            return;
        auto loaderClass = env->GetObjectClass(loader);
        if (!Check(java) || !loaderClass)
            return;
        auto loadClass =
            env->GetMethodID(loaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
        if (!Check(java) || !loadClass)
            return;
        auto name = env->NewStringUTF("org.raygame.SpeechBridge");
        if (!Check(java) || !name)
            return;
        auto clazz = static_cast<jclass>(env->CallObjectMethod(loader, loadClass, name));
        if (!Check(java) || !clazz) {
            error = "SpeechBridge is missing from the APK classes.dex";
            return;
        }
        auto constructor = env->GetMethodID(clazz, "<init>", "(Landroid/app/Activity;)V");
        if (!Check(java) || !constructor)
            return;
        stateMethod = env->GetMethodID(clazz, "getState", "()I");
        if (!Check(java) || !stateMethod)
            return;
        errorMethod = env->GetMethodID(clazz, "getError", "()Ljava/lang/String;");
        if (!Check(java) || !errorMethod)
            return;
        speakMethod = env->GetMethodID(clazz, "speak", "(Ljava/lang/String;Ljava/lang/String;FF)Z");
        if (!Check(java) || !speakMethod)
            return;
        stopMethod = env->GetMethodID(clazz, "stop", "()V");
        if (!Check(java) || !stopMethod)
            return;
        closeMethod = env->GetMethodID(clazz, "close", "()V");
        if (!Check(java) || !constructor || !stateMethod || !errorMethod || !speakMethod ||
            !stopMethod || !closeMethod)
            return;
        auto object = env->NewObject(clazz, constructor, activity->clazz);
        if (!Check(java) || !object)
            return;
        bridge = env->NewGlobalRef(object);
        Check(java);
    }
    ~AndroidSpeech() override {
        JavaFrame java(vm);
        if (bridge && java.env) {
            java.env->CallVoidMethod(bridge, closeMethod);
            java.Good();
            java.env->DeleteGlobalRef(bridge);
        }
    }
    SpeechState State() override {
        if (!bridge)
            return SpeechState::Unavailable;
        JavaFrame java(vm);
        if (!Check(java))
            return SpeechState::Unavailable;
        int value = java.env->CallIntMethod(bridge, stateMethod);
        return Check(java) && value >= 0 && value <= 4 ? static_cast<SpeechState>(value)
                                                       : SpeechState::Unavailable;
    }
    std::string Error() override {
        if (!bridge || !error.empty())
            return error;
        JavaFrame java(vm);
        if (!Check(java))
            return error;
        auto message = static_cast<jstring>(java.env->CallObjectMethod(bridge, errorMethod));
        if (!Check(java) || !message)
            return error;
        const char* value = java.env->GetStringUTFChars(message, nullptr);
        if (!value) {
            Check(java);
            return error;
        }
        std::string result(value);
        java.env->ReleaseStringUTFChars(message, value);
        return result;
    }
    bool Speak(const std::u16string& text, const SpeechOptions& options) override {
        error.clear();
        if (!bridge) {
            error = "Android TTS bridge is unavailable";
            return false;
        }
        JavaFrame java(vm);
        if (!Check(java))
            return false;
        auto string = java.env->NewString(reinterpret_cast<const jchar*>(text.data()),
                                          static_cast<jsize>(text.size()));
        if (!Check(java) || !string)
            return false;
        auto language = java.env->NewStringUTF(options.language.c_str());
        if (!Check(java) || !string || !language)
            return false;
        bool accepted = java.env->CallBooleanMethod(bridge, speakMethod, string, language,
                                                    options.rate, options.volume);
        return Check(java) && accepted;
    }
    void Stop() override {
        if (!bridge)
            return;
        JavaFrame java(vm);
        if (!Check(java))
            return;
        java.env->CallVoidMethod(bridge, stopMethod);
        Check(java);
    }
};
} // namespace
std::unique_ptr<SpeechBackend> CreateSpeechBackend() {
    return std::make_unique<AndroidSpeech>();
}
} // namespace RayGame::detail
#endif
