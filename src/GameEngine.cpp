#include "GameEngine.h"
#include "WindowStyle.h"

namespace RayGame {

void GameEngine::RefreshWindowTheme(bool force) {
#ifdef _WIN32
    detail::UpdateWindowTheme(GetWindowHandle(), force);
#else
    (void)force;
#endif
}

} // namespace RayGame

