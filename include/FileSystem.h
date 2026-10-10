#pragma once
#include "Export.h"
#include <filesystem>
#include <string>

namespace RayGame {
class RAYGAME_API FileSystem {
public:
    // appId is one filename component, e.g. "raygame.neon_breakout".
    static std::filesystem::path UserDataDirectory(const std::string& appId);
    // Includes APK assets on Android; there paths are relative to the assets root.
    static bool AssetExists(const std::string& path);
    // Desktop files or APK assets (relative to the APK assets root on Android).
    // Failures leave text unchanged, including missing or oversized assets.
    static bool ReadAssetText(const std::string& path, std::string& text,
                              size_t maxBytes = 1024 * 1024);
    static bool ReadText(const std::filesystem::path& path, std::string& text,
                         size_t maxBytes = 1024 * 1024);
    // Writes/syncs a unique sibling, then atomically replaces the destination.
    // A failed write/replacement leaves the previous destination intact.
    static bool WriteAtomic(const std::filesystem::path& path, const std::string& text,
                            bool backup = true);
};
}
