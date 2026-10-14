#pragma once

namespace UG {

enum class MenuPage { Title, Levels, Heroes, Kaiju, Help };

void GoToMenu(MenuPage page = MenuPage::Title);
void GoToBattle(int level);
void ToggleFullscreenMode();
void SetGameSpeed(float scale); // simulation time scale; 0 pauses gameplay but not the UI
void QuitGame();

} // namespace UG
