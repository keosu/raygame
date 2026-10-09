#if defined(__ANDROID__)
#include "AndroidPlatform.h"
#include <android_native_app_glue.h>
#include <android/asset_manager.h>

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
