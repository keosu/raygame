#pragma once

namespace UG {

enum class MenuPage { Title, Levels, Heroes, Kaiju, Help, Prepare };

void GoToMenu(MenuPage page = MenuPage::Title);
void GoToBattle(int level); // kSurvivalIndex starts survival mode
void PrepareLevel(int level); // the next MenuPage::Prepare picks a lineup for this level
void ToggleFullscreenMode();
void SetGameSpeed(float scale); // simulation time scale; 0 pauses gameplay but not the UI
void QuitGame();
bool ScriptedCapture(); // --capture tour: the window may never get focus

} // namespace UG
