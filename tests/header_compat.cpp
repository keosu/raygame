// Windows must resolve <time.h> to the CRT, including before the engine header.
#include <ctime>
#include <chrono>
#include <filesystem>
#include "RayGame.h"

static_assert(sizeof(std::time_t) > 0, "CRT time definitions must remain available");
float HeaderCompatibilityCheck() {
    using namespace RayGame;
    return Lerp(0.0f, 1.0f, Clamp(0.5f, 0.0f, 1.0f));
}
