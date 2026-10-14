#pragma once

#include "Core.h"
#include "Diagnostics.h"
#include <rlgl.h>
#include <array>
#include <stdexcept>

namespace RayGame {

enum class CanvasScaleMode { Fit, Expand };
enum class CanvasQuality { Standard = 1, Smooth = 2 };

// Both policies scale uniformly. Expand adds visible game space on the longer axis.
struct CanvasViewport {
    Rectangle bounds = {0, 0, 1, 1};
    float scale = 1;
    Vector2 logicalSize = {1, 1};

    static CanvasViewport Fit(int canvasWidth, int canvasHeight, int windowWidth,
                              int windowHeight) {
        canvasWidth = std::max(1, canvasWidth);
        canvasHeight = std::max(1, canvasHeight);
        windowWidth = std::max(1, windowWidth);
        windowHeight = std::max(1, windowHeight);
        float scale =
            std::min((float)windowWidth / canvasWidth, (float)windowHeight / canvasHeight);
        float width = canvasWidth * scale, height = canvasHeight * scale;
        return {{(windowWidth - width) / 2, (windowHeight - height) / 2, width, height},
                scale,
                {(float)canvasWidth, (float)canvasHeight}};
    }

    static CanvasViewport Calculate(int width, int height, int windowWidth, int windowHeight,
                                    CanvasScaleMode mode) {
        auto result = Fit(width, height, windowWidth, windowHeight);
        if (mode == CanvasScaleMode::Expand) {
            result.bounds = {0, 0, (float)std::max(1, windowWidth),
                             (float)std::max(1, windowHeight)};
            result.logicalSize = {result.bounds.width / result.scale,
                                  result.bounds.height / result.scale};
        }
        return result;
    }

    Vector2 ToCanvas(Vector2 screen) const {
        return {(screen.x - bounds.x) / scale, (screen.y - bounds.y) / scale};
    }
    Vector2 ToScreen(Vector2 canvas) const {
        return {bounds.x + canvas.x * scale, bounds.y + canvas.y * scale};
    }
    bool Contains(Vector2 screen) const {
        return CheckCollisionPointRec(screen, bounds);
    }
};

class RAYGAME_API Display {
public:
    static void Configure(int width, int height, CanvasScaleMode mode = CanvasScaleMode::Fit) {
        canvasWidth = std::max(1, width);
        canvasHeight = std::max(1, height);
        scaleMode = mode;
        viewport = CanvasViewport::Fit(canvasWidth, canvasHeight, canvasWidth, canvasHeight);
    }

    static void Refresh() {
        // Minimized windows retain their last valid layout.
        if (::GetScreenWidth() <= 0 || ::GetScreenHeight() <= 0)
            return;
        viewport = CanvasViewport::Calculate(canvasWidth, canvasHeight, ::GetScreenWidth(),
                                             ::GetScreenHeight(), scaleMode);
    }

    static const CanvasViewport& GetViewport() {
        return viewport;
    }
    static Vector2 GetSize() {
        return viewport.logicalSize;
    }
    static float GetWidth() {
        return viewport.logicalSize.x;
    }
    static float GetHeight() {
        return viewport.logicalSize.y;
    }
    static Vector2 ScreenToCanvas(Vector2 screen) {
        return viewport.ToCanvas(screen);
    }
    static Vector2 CanvasToScreen(Vector2 canvas) {
        return viewport.ToScreen(canvas);
    }
    static Texture2D GetCanvasTexture() {
        return canvas.texture;
    }
    // Smooth renders at twice the native sampling density, within the 4096 texture cap.
    static void SetQuality(CanvasQuality quality) {
        canvasQuality = quality;
        failedWidth = failedHeight = 0;
    }
    static CanvasQuality GetQuality() {
        return canvasQuality;
    }

    static void Begin() {
        // Render above native resolution, then downsample. Text remains sharp in
        // fullscreen/HiDPI. Quantized sizes avoid reallocating for each resize pixel.
        float fit =
            std::min((float)GetRenderWidth() / GetWidth(), (float)GetRenderHeight() / GetHeight());
        int quality = std::max(1, (int)std::ceil(fit * static_cast<int>(canvasQuality)));
        float desiredWidth = GetWidth() * quality, desiredHeight = GetHeight() * quality;
        float limit = std::min(1.0f, 4096.0f / std::max(desiredWidth, desiredHeight));
        int width = std::min(4096, std::max(64, (int)std::ceil(desiredWidth * limit / 64) * 64));
        int height = std::min(4096, std::max(64, (int)std::ceil(desiredHeight * limit / 64) * 64));
        if ((canvas.texture.width != width || canvas.texture.height != height) &&
            (failedWidth != width || failedHeight != height)) {
            auto next = LoadRenderTexture(width, height);
            if (IsRenderTextureValid(next)) {
                SetTextureFilter(next.texture, TEXTURE_FILTER_BILINEAR);
                if (canvas.id)
                    UnloadRenderTexture(canvas);
                canvas = next;
                failedWidth = failedHeight = 0;
            } else {
                if (next.id)
                    UnloadRenderTexture(next);
                failedWidth = width;
                failedHeight = height;
                Diagnostics::Write(LogLevel::Error,
                                   "Cannot allocate game canvas; retaining the previous canvas");
            }
        }
        if (!canvas.id)
            throw std::runtime_error("Cannot create a game canvas");
        BeginCanvasMode();
    }

    static void End() {
        inFrame = false;
        EndTextureMode();
        Rectangle window{0, 0, (float)::GetScreenWidth(), (float)::GetScreenHeight()};
        bool margins = viewport.bounds.width < window.width - 0.5f ||
                       viewport.bounds.height < window.height - 0.5f;
        if (margins && marginStyle == MarginStyle::BlurredCanvas)
            PrepareMarginBlur(canvas.texture);
        BeginDrawing();
        ClearBackground(marginColor);
        if (margins)
            DrawMarginFill(window, viewport.bounds);
        PresentOpaque(canvas.texture, viewport.bounds);
        EndDrawing();
    }

    // What fills the bars around a Fit canvas whose aspect differs from the window.
    // BlurredCanvas stretches a soft, dimmed copy of the current frame across the window,
    // so the margins continue the scene's colors instead of showing a flat bar.
    enum class MarginStyle { Solid, BlurredCanvas };
    static void SetMarginStyle(MarginStyle style, float brightness = 0.45f) {
        marginStyle = style;
        if (std::isfinite(brightness))
            marginBrightness = Clamp(brightness, 0.0f, 1.0f);
    }
    static MarginStyle GetMarginStyle() {
        return marginStyle;
    }
    static void SetMarginColor(Color color) {
        marginColor = color;
    }

    // Building blocks of End, exposed for custom presentation and tests. Prepare must run
    // outside any texture mode; DrawMarginFill draws into the current target, covering
    // window (the area outside ounds is what stays visible).
    static void PrepareMarginBlur(Texture2D source) {
        Texture2D current = source;
        for (int i = 0; i < 3; ++i) {
            int w = std::max(1, source.width >> (2 * (i + 1))),
                h = std::max(1, source.height >> (2 * (i + 1)));
            RenderTexture2D& stage = blurStages[i];
            if (stage.texture.width != w || stage.texture.height != h) {
                if (stage.id)
                    UnloadRenderTexture(stage);
                stage = LoadRenderTexture(w, h);
                if (!IsRenderTextureValid(stage)) {
                    stage = {};
                    blurReady = false;
                    return;
                }
                SetTextureFilter(stage.texture, TEXTURE_FILTER_BILINEAR);
            }
            BeginTextureMode(stage);
            PresentOpaque(current, {0, 0, (float)w, (float)h});
            EndTextureMode();
            current = stage.texture;
        }
        blurReady = true;
    }
    static void DrawMarginFill(Rectangle window, Rectangle bounds) {
        DrawRectangleRec(window, marginColor);
        if (marginStyle != MarginStyle::BlurredCanvas || !blurReady || bounds.width <= 0 ||
            bounds.height <= 0)
            return;
        // Cover the window with the canvas aspect, centered like the canvas itself.
        float scale = std::max(window.width / bounds.width, window.height / bounds.height);
        float w = bounds.width * scale, h = bounds.height * scale;
        Rectangle cover{bounds.x + bounds.width / 2 - w / 2, bounds.y + bounds.height / 2 - h / 2,
                        w, h};
        auto level = (unsigned char)std::lround(255 * marginBrightness);
        PresentOpaque(blurStages[2].texture, cover, {level, level, level, 255});
    }

    // True between Begin and End, i.e. while scene and UI rendering target the canvas.
    static bool InFrame() {
        return inFrame;
    }

    // Copies a render-texture color attachment without blending. Translucent drawing
    // lowers a canvas's alpha; presenting with blending would mix in whatever lies below.
    static void PresentOpaque(Texture2D texture, Rectangle dest, Color tint = WHITE) {
        rlDrawRenderBatchActive();
        rlDisableColorBlend();
        DrawTexturePro(texture, {0, 0, (float)texture.width, -(float)texture.height}, dest, {0, 0},
                       0, tint);
        rlDrawRenderBatchActive();
        rlEnableColorBlend();
    }

    // Redirects drawing into another render target, e.g. to bake a texture during Render.
    // raylib cannot nest texture modes, so the canvas is suspended and its projection
    // restored afterwards. Matrices pushed before the scope are not carried into it.
    class OffscreenScope {
    public:
        explicit OffscreenScope(RenderTexture2D target) : resume(inFrame) {
            if (resume) {
                inFrame = false;
                EndTextureMode();
            }
            BeginTextureMode(target);
        }
        ~OffscreenScope() {
            EndTextureMode();
            if (resume)
                BeginCanvasMode();
        }
        OffscreenScope(const OffscreenScope&) = delete;
        OffscreenScope& operator=(const OffscreenScope&) = delete;

    private:
        bool resume;
    };

    static void Shutdown() {
        if (canvas.id)
            UnloadRenderTexture(canvas);
        canvas = {};
        for (auto& stage : blurStages)
            if (stage.id)
                UnloadRenderTexture(stage);
        blurStages = {};
        blurReady = false;
        viewport = {};
        failedWidth = failedHeight = 0;
        inFrame = false;
        canvasQuality = CanvasQuality::Standard;
    }

private:
    static void BeginCanvasMode() {
        BeginTextureMode(canvas);
        // Scale the projection so nested scene cameras keep working in game units.
        rlMatrixMode(RL_PROJECTION);
        rlLoadIdentity();
        rlOrtho(0, GetWidth(), GetHeight(), 0, RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();
        inFrame = true;
    }

    static inline bool inFrame = false;
    static inline MarginStyle marginStyle = MarginStyle::Solid;
    static inline float marginBrightness = 0.45f;
    static inline Color marginColor = {4, 7, 16, 255};
    static inline std::array<RenderTexture2D, 3> blurStages{};
    static inline bool blurReady = false;
    static inline int canvasWidth = 1, canvasHeight = 1;
    static inline CanvasScaleMode scaleMode = CanvasScaleMode::Fit;
    static inline CanvasQuality canvasQuality = CanvasQuality::Standard;
    static inline int failedWidth = 0, failedHeight = 0;
    static inline CanvasViewport viewport;
    static inline RenderTexture2D canvas = {};
};

} // namespace RayGame
