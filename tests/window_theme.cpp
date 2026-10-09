// Inspect the actual DWM state without including windows.h alongside raylib.h.
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>
#include <cstdio>
#endif

bool NativeWindowHasIcon(void* window) {
#ifdef _WIN32
    return SendMessageW(static_cast<HWND>(window), WM_GETICON, ICON_BIG, 0) != 0
        && SendMessageW(static_cast<HWND>(window), WM_GETICON, ICON_SMALL, 0) != 0;
#else
    (void)window;
    return true;
#endif
}

bool NativeWindowThemeMatchesSystem(void* window) {
#ifdef _WIN32
    BOOL dark = FALSE;
    HRESULT result = DwmGetWindowAttribute(static_cast<HWND>(window), 20, &dark, sizeof(dark));
    if (FAILED(result))
        result = DwmGetWindowAttribute(static_cast<HWND>(window), 19, &dark, sizeof(dark));
    if (FAILED(result)) {
        std::printf("SKIP: native dark title bars are unavailable on this Windows version\n");
        return true;
    }
    DWORD light = 1;
    DWORD size = sizeof(light);
    constexpr auto key = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
    if (RegGetValueW(HKEY_CURRENT_USER, key, L"AppsUseLightTheme", RRF_RT_REG_DWORD,
                     nullptr, &light, &size) != ERROR_SUCCESS) {
        light = 1;
        size = sizeof(light);
        RegGetValueW(HKEY_CURRENT_USER, key, L"SystemUsesLightTheme", RRF_RT_REG_DWORD,
                     nullptr, &light, &size);
    }
    HIGHCONTRASTW contrast = {};
    contrast.cbSize = sizeof(contrast);
    bool highContrast = SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0)
        && (contrast.dwFlags & HCF_HIGHCONTRASTON);
    bool expectedDark = light == 0 && !highContrast;
    std::printf("Native title bar: dark=%d, expected=%d\n", dark != FALSE, expectedDark);
    return (dark != FALSE) == expectedDark;
#else
    (void)window;
    return true;
#endif
}
