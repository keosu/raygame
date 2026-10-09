#pragma once

#if defined(_WIN32) && defined(RAYGAME_SHARED)
#ifdef RAYGAME_EXPORTS
#define RAYGAME_API __declspec(dllexport)
#else
#define RAYGAME_API __declspec(dllimport)
#endif
#else
#define RAYGAME_API
#endif
