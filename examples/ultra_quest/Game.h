#pragma once

#include "Common.h"

namespace UQ {

void GoToTitle();
void GoToExplore();
void GoToTransform();
// `story` plays the landing intro and the epilogue; a rematch from the result page skips them.
void GoToFight(bool story);
void QuitGame();
void ToggleFullscreenMode();
void SetGameSpeed(float scale);

} // namespace UQ
