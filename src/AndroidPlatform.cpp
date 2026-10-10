#if defined(__ANDROID__)
#include "AndroidPlatform.h"
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <algorithm>
#include <memory>

namespace {
android_app* currentApp = nullptr;
}

namespace RayGame::detail {
const char* AndroidDataDirectory() {
    return currentApp && currentApp->activity ? currentApp->activity->internalDataPath : nullptr;
}
bool AndroidAssetExists(const char* path) {
    if (!currentApp || !currentApp->activity) return false;
    AAsset* asset = AAssetManager_open(currentApp->activity->assetManager, path, AASSET_MODE_UNKNOWN);
    if (!asset) return false;
    AAsset_close(asset);
    return true;
}
bool AndroidReadAssetText(const char* path, std::string& text, size_t maxBytes) {
    if (!currentApp || !currentApp->activity) return false;
    std::unique_ptr<AAsset, decltype(&AAsset_close)> asset(
        AAssetManager_open(currentApp->activity->assetManager, path, AASSET_MODE_STREAMING), AAsset_close);
    if (!asset) return false;
    try {
        auto length = AAsset_getLength64(asset.get());
        if (length < 0 || static_cast<unsigned long long>(length) > maxBytes) return false;
        std::string value(static_cast<size_t>(length), '\0');
        size_t offset = 0;
        while (offset < value.size()) {
            int count = AAsset_read(asset.get(), value.data() + offset,
                                   std::min<size_t>(value.size() - offset, 1024 * 1024));
            if (count <= 0) return false;
            offset += static_cast<size_t>(count);
        }
        text = std::move(value);
        return true;
    } catch (...) { return false; }
}
}

// raylib owns android_main and the event loop. Capture NativeActivity context
// before it calls the game's main(), so startup logging can use private storage.
extern "C" void __real_android_main(android_app* app);
extern "C" void __wrap_android_main(android_app* app) {
    currentApp = app;
    __real_android_main(app);
    currentApp = nullptr;
}
#endif
