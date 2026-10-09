#pragma once

// Keep native platform headers separate from raylib's conflicting Win32 names.
namespace RayGame::detail {
void UpdateWindowTheme(void* window, bool force);
}
