#pragma once

#include "Core.h"
#include <rlgl.h>

namespace RayGame {

// Fits a fixed game canvas inside a window without stretching or cropping.
struct CanvasViewport {
    Rectangle bounds = {0, 0, 1, 1};
    float scale = 1;

    static CanvasViewport Fit(int canvasWidth, int canvasHeight, int windowWidth, int windowHeight) {
        canvasWidth = std::max(1, canvasWidth);
        canvasHeight = std::max(1, canvasHeight);
        windowWidth = std::max(1, windowWidth);
        windowHeight = std::max(1, windowHeight);
        float scale = std::min((float)windowWidth / canvasWidth, (float)windowHeight / canvasHeight);
        float width = canvasWidth * scale, height = canvasHeight * scale;
        return {{(windowWidth - width) / 2, (windowHeight - height) / 2, width, height}, scale};
    }

    Vector2 ToCanvas(Vector2 screen) const {
        return {(screen.x - bounds.x) / scale, (screen.y - bounds.y) / scale};
    }
    Vector2 ToScreen(Vector2 canvas) const {
        return {bounds.x + canvas.x * scale, bounds.y + canvas.y * scale};
    }
    bool Contains(Vector2 screen) const { return CheckCollisionPointRec(screen, bounds); }
};

class RAYGAME_API Display {
public:
    static void Configure(int width, int height) {
        canvasWidth = std::max(1, width);
        canvasHeight = std::max(1, height);
        viewport = CanvasViewport::Fit(canvasWidth, canvasHeight, canvasWidth, canvasHeight);
    }

    static void Refresh() {
        viewport = CanvasViewport::Fit(canvasWidth, canvasHeight, ::GetScreenWidth(), ::GetScreenHeight());
    }

    static const CanvasViewport& GetViewport() { return viewport; }
    static Vector2 ScreenToCanvas(Vector2 screen) { return viewport.ToCanvas(screen); }
    static Vector2 CanvasToScreen(Vector2 canvas) { return viewport.ToScreen(canvas); }
    static Texture2D GetCanvasTexture() { return canvas.texture; }

    static void Begin() {
        Refresh();
        // Render above native resolution, then downsample. Text remains sharp in
        // fullscreen/HiDPI; integer tiers avoid reallocating on every resize pixel.
        float fit = std::min((float)GetRenderWidth() / canvasWidth, (float)GetRenderHeight() / canvasHeight);
        int quality = std::max(1, (int)std::ceil(fit));
        int width = canvasWidth * quality, height = canvasHeight * quality;
        if (canvas.texture.width != width || canvas.texture.height != height) {
            if (canvas.id) UnloadRenderTexture(canvas);
            canvas = LoadRenderTexture(width, height);
            SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);
        }
        BeginTextureMode(canvas);
        // Scale the projection so nested scene cameras keep working in game units.
        rlMatrixMode(RL_PROJECTION);
        rlLoadIdentity();
        rlOrtho(0, canvasWidth, canvasHeight, 0, RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();
    }

    static void End() {
        EndTextureMode();
        BeginDrawing();
        ClearBackground({4, 7, 16, 255});
        DrawTexturePro(canvas.texture, {0, 0, (float)canvas.texture.width, -(float)canvas.texture.height},
                       viewport.bounds, {0, 0}, 0, WHITE);
        EndDrawing();
    }

    static void Shutdown() {
        if (canvas.id) UnloadRenderTexture(canvas);
        canvas = {};
        viewport = {};
    }

private:
    static inline int canvasWidth = 1, canvasHeight = 1;
    static inline CanvasViewport viewport;
    static inline RenderTexture2D canvas = {};
};

}
