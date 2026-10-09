#include "FileSystem.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

bool AtomicReplacementPreservesLockedFile(const std::filesystem::path& path) {
    if (!RayGame::FileSystem::WriteAtomic(path, "previous contents", false)) return false;
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    bool replaced = RayGame::FileSystem::WriteAtomic(path, "new contents", true);
    CloseHandle(file);
    std::string content;
    return !replaced && RayGame::FileSystem::ReadText(path, content) && content == "previous contents";
}
#endif
