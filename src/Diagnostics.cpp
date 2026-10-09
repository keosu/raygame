#include "Diagnostics.h"
#include <raylib.h>
#include <mutex>
#include <algorithm>
#include <fstream>
#include <cstdio>
#include <ctime>
#include <cstdarg>
#if defined(__ANDROID__)
#include <android/log.h>
#endif

namespace RayGame {
namespace {
std::mutex logMutex;
std::ofstream output;
std::filesystem::path logPath;
bool consoleOutput = false;
bool callbackInstalled = false;
size_t bytesWritten = 0, byteLimit = 1024 * 1024;

void OpenLog() {
    output.open(logPath, std::ios::binary | std::ios::app);
    std::error_code ec;
    bytesWritten = static_cast<size_t>(std::filesystem::file_size(logPath, ec));
    if (ec) bytesWritten = 0;
}

void RaylibLog(int level, const char* format, va_list args) {
    char buffer[4096];
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    auto severity = level >= LOG_ERROR ? LogLevel::Error : level >= LOG_WARNING ? LogLevel::Warning : LogLevel::Info;
    Diagnostics::Write(severity, buffer);
}
}

bool Diagnostics::Initialize(const std::filesystem::path& file, bool console, size_t maxBytes) noexcept {
    try {
        std::lock_guard<std::mutex> lock(logMutex);
        output.close(); output.clear();
        consoleOutput = console;
        logPath = file;
        byteLimit = std::max<size_t>(maxBytes, 1024);
        if (!file.parent_path().empty()) std::filesystem::create_directories(file.parent_path());
        OpenLog();
        return output.good();
    } catch (...) { return false; }
}

void Diagnostics::Write(LogLevel level, const std::string& message) noexcept {
#if defined(__ANDROID__)
    int priority = level == LogLevel::Error ? ANDROID_LOG_ERROR : level == LogLevel::Warning ? ANDROID_LOG_WARN : ANDROID_LOG_INFO;
    __android_log_write(priority, "raygame", message.c_str());
#endif
    try {
        std::lock_guard<std::mutex> lock(logMutex);
        const char* label = level == LogLevel::Error ? "ERROR" : level == LogLevel::Warning ? "WARN" : "INFO";
        std::time_t now = std::time(nullptr);
        std::tm utc = {};
#ifdef _WIN32
        gmtime_s(&utc, &now);
#else
        gmtime_r(&now, &utc);
#endif
        char timestamp[32];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &utc);
        std::string line = std::string(timestamp) + " [" + label + "] " + message.substr(0, 8192) + "\n";
        if (consoleOutput) std::fputs(line.c_str(), stderr);
        if (!output.is_open()) return;
        if (bytesWritten + line.size() > byteLimit) {
            output.close();
            auto previous = logPath; previous += ".previous";
            std::error_code ec;
            std::filesystem::remove(previous, ec);
            ec.clear();
            std::filesystem::rename(logPath, previous, ec);
            // If rotation fails, truncate the active log to keep disk use bounded.
            if (ec) { output.open(logPath, std::ios::binary | std::ios::trunc); output.close(); }
            output.clear(); OpenLog();
        }
        output << line;
        output.flush();
        bytesWritten += line.size();
    } catch (...) { /* Diagnostic failure must not break teardown. */ }
}

void Diagnostics::InstallRaylibCallback() {
    SetTraceLogCallback(RaylibLog);
    SetTraceLogLevel(LOG_INFO);
    callbackInstalled = true;
}

void Diagnostics::Shutdown() noexcept {
    try {
        std::lock_guard<std::mutex> lock(logMutex);
        if (callbackInstalled) { SetTraceLogCallback(nullptr); callbackInstalled = false; }
        output.close(); output.clear();
        consoleOutput = false;
    } catch (...) {}
}
}
