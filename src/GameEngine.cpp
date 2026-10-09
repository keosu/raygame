#include "GameEngine.h"
#include "WindowStyle.h"

namespace RayGame {

void GameEngine::RefreshWindowTheme(bool force) {
    detail::UpdateWindowTheme(GetWindowHandle(), force);
}

} // namespace RayGame

