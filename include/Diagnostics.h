#pragma once
#include "Export.h"
#include <filesystem>
#include <string>

namespace RayGame {
enum class LogLevel { Info, Warning, Error };

class RAYGAME_API Diagnostics {
public:
    // One active log, rotated to .previous at 1 MiB by default.
    static bool Initialize(const std::filesystem::path& file, bool console = false,
                           size_t maxBytes = 1024 * 1024) noexcept;
    static void Write(LogLevel level, const std::string& message) noexcept;
    static void InstallRaylibCallback();
    static void Shutdown() noexcept;
};
}
