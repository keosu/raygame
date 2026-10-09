#include "WindowStyle.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>

namespace {
bool UseDarkTitleBar() {
    HIGHCONTRASTW contrast = {};
    contrast.cbSize = sizeof(contrast);
    if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0)
        && (contrast.dwFlags & HCF_HIGHCONTRASTON)) return false;

    constexpr auto key = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    DWORD lightTheme = 1;
    DWORD size = sizeof(lightTheme);
    if (RegGetValueW(HKEY_CURRENT_USER, key, L"AppsUseLightTheme", RRF_RT_REG_DWORD,
                     nullptr, &lightTheme, &size) != ERROR_SUCCESS) {
        lightTheme = 1;
        size = sizeof(lightTheme);
        RegGetValueW(HKEY_CURRENT_USER, key, L"SystemUsesLightTheme", RRF_RT_REG_DWORD,
                     nullptr, &lightTheme, &size);
    }
    return lightTheme == 0;
}
}
#endif

namespace RayGame::detail {
void UpdateWindowTheme(void* window, bool force) {
#ifdef _WIN32
    if (!window) return;
    static HWND previousWindow = nullptr;
    static BOOL previousDark = FALSE;
    static ULONGLONG lastCheck = 0;
    HWND handle = static_cast<HWND>(window);
    ULONGLONG now = GetTickCount64();
    // Theme changes are rare; avoid registry work every frame and leave GLFW's
    // window procedure intact. Force a refresh when the native frame is restored.
    if (!force && handle == previousWindow && now - lastCheck < 500) return;
    lastCheck = now;
    BOOL dark = UseDarkTitleBar() ? TRUE : FALSE;
    if (force || handle != previousWindow || dark != previousDark) {
        // 20 is DWMWA_USE_IMMERSIVE_DARK_MODE; early Windows 10 used 19.
        if (FAILED(DwmSetWindowAttribute(handle, 20, &dark, sizeof(dark))))
            DwmSetWindowAttribute(handle, 19, &dark, sizeof(dark));
        RedrawWindow(handle, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME);
    }
    previousWindow = handle;
    previousDark = dark;
#else
    (void)window;
    (void)force;
#endif
}
}
