#pragma once

#if defined(__ANDROID__)
namespace RayGame::detail {
const char* AndroidDataDirectory();
bool AndroidAssetExists(const char* path);
}
#endif
