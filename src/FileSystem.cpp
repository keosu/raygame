#include "FileSystem.h"
#include <atomic>
#include <algorithm>
#include <mutex>
#include <cstdlib>
#include <fstream>
#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#endif

namespace RayGame {
namespace {
namespace fs = std::filesystem;
std::atomic<unsigned long long> serial{0};
std::recursive_mutex writeMutex;

fs::path Sibling(const fs::path& path, const char* suffix) {
    auto result = path;
    result += suffix;
    return result;
}

bool Replace(const fs::path& from, const fs::path& to) {
#ifdef _WIN32
    return MoveFileExW(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    if (::rename(from.c_str(), to.c_str()) != 0) return false;
    auto parent = to.parent_path().empty() ? fs::path(".") : to.parent_path();
    int directory = ::open(parent.c_str(), O_RDONLY | O_DIRECTORY);
    if (directory >= 0) { ::fsync(directory); ::close(directory); }
    return true;
#endif
}

bool WriteSynced(const fs::path& path, const std::string& text, bool& created) {
#ifdef _WIN32
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    created = true;
    size_t offset = 0;
    bool ok = true;
    while (offset < text.size()) {
        DWORD written = 0;
        DWORD amount = static_cast<DWORD>(std::min<size_t>(text.size() - offset, 1024 * 1024));
        if (!WriteFile(file, text.data() + offset, amount, &written, nullptr) || written == 0) { ok = false; break; }
        offset += written;
    }
    if (ok) ok = FlushFileBuffers(file) != 0;
    if (!CloseHandle(file)) ok = false;
    return ok;
#else
    int file = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (file < 0) return false;
    created = true;
    size_t offset = 0;
    bool ok = true;
    while (offset < text.size()) {
        auto written = ::write(file, text.data() + offset, text.size() - offset);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) { ok = false; break; }
        offset += static_cast<size_t>(written);
    }
    if (ok) ok = ::fsync(file) == 0;
    if (::close(file) != 0) ok = false;
    return ok;
#endif
}
}

std::filesystem::path FileSystem::UserDataDirectory(const std::string& appId) {
    if (appId.empty() || appId == "." || appId == ".." ||
        appId.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") != std::string::npos)
        throw std::invalid_argument("Application ID must be a single ASCII filename component");
    fs::path base;
#ifdef _WIN32
    PWSTR folder = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &folder))) base = folder;
    CoTaskMemFree(folder);
#else
    const char* homeDirectory = std::getenv("HOME");
#ifdef __APPLE__
    if (homeDirectory) base = fs::path(homeDirectory) / "Library/Application Support";
#else
    const char* xdgData = std::getenv("XDG_DATA_HOME");
    if (xdgData && fs::path(xdgData).is_absolute()) base = xdgData;
    else if (homeDirectory) base = fs::path(homeDirectory) / ".local/share";
#endif
#endif
    if (base.empty()) throw std::runtime_error("Cannot locate the user data directory");
    auto result = base / appId;
    fs::create_directories(result);
    return result;
}

bool FileSystem::ReadText(const fs::path& path, std::string& text, size_t maxBytes) {
    try {
        std::ifstream stream(path, std::ios::binary | std::ios::ate);
        if (!stream) return false;
        auto size = stream.tellg();
        if (size < 0 || static_cast<unsigned long long>(size) > maxBytes) return false;
        std::string value(static_cast<size_t>(size), '\0');
        stream.seekg(0);
        if (!value.empty() && !stream.read(value.data(), static_cast<std::streamsize>(value.size()))) return false;
        text = std::move(value);
        return true;
    } catch (...) { return false; }
}

bool FileSystem::WriteAtomic(const fs::path& path, const std::string& text, bool backup) {
    fs::path temp;
    bool ownsTemp = false;
    try {
        std::lock_guard<std::recursive_mutex> lock(writeMutex);
        if (path.empty() || path.filename().empty()) return false;
        std::error_code ec;
        if (fs::exists(path, ec) && !fs::is_regular_file(path, ec)) return false;
        if (!path.parent_path().empty()) fs::create_directories(path.parent_path());
#ifdef _WIN32
        auto pid = GetCurrentProcessId();
#else
        auto pid = ::getpid();
#endif
        // CREATE_NEW/O_EXCL prevents clobbering an existing temporary file.
        do {
            temp = Sibling(path, (".tmp-" + std::to_string(pid) + "-" + std::to_string(++serial)).c_str());
        } while (fs::exists(temp));
        if (!WriteSynced(temp, text, ownsTemp)) {
            if (ownsTemp) fs::remove(temp, ec);
            return false;
        }
        if (backup && fs::exists(path)) {
            std::string old;
            if (!ReadText(path, old) || !WriteAtomic(Sibling(path, ".bak"), old, false)) {
                fs::remove(temp, ec);
                return false;
            }
        }
        bool ok = Replace(temp, path);
        if (!ok) fs::remove(temp, ec);
        return ok;
    } catch (...) {
        std::error_code ec;
        if (ownsTemp) fs::remove(temp, ec);
        return false;
    }
}
}
