#pragma once

#if defined(__ANDROID__)
#include <string>
#include <android/native_activity.h>
namespace RayGame::detail {
ANativeActivity* AndroidActivity();
const char* AndroidDataDirectory();
bool AndroidAssetExists(const char* path);
bool AndroidReadAssetText(const char* path, std::string& text, size_t maxBytes);
}
#endif
