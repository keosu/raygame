#pragma once

#if defined(__ANDROID__)
#include <string>
namespace RayGame::detail {
const char* AndroidDataDirectory();
bool AndroidAssetExists(const char* path);
bool AndroidReadAssetText(const char* path, std::string& text, size_t maxBytes);
}
#endif
