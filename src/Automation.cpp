#include "Automation.h"

namespace RayGame {

namespace {
bool ExportOpaque(Image image, Vector2 size, const std::filesystem::path& path) {
    if (!IsImageValid(image)) return false;
    ImageFlipVertical(&image);
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8); // the window shows the canvas opaque
    int width = (int)std::lround(size.x), height = (int)std::lround(size.y);
    if (width > 0 && height > 0 && (image.width != width || image.height != height))
        ImageResize(&image, width, height);
    bool ok = ExportImage(image, path.u8string().c_str());
    UnloadImage(image);
    return ok;
}
} // namespace

bool Automation::Start(const std::filesystem::path& path, Options value) {
    Stop();
    std::error_code error;
    std::filesystem::create_directories(path, error);
    if (!std::filesystem::is_directory(path, error)) return false;
    directory = path;
    options = value;
    active = true;
    Input::SetCaptureFilter([](InputFrame& input) { Overlay(input); });
    return true;
}

void Automation::Stop() {
    if (active) Input::SetCaptureFilter({});
    active = false;
    frame = 0;
    timeline.clear();
    everyFrame.clear();
    clicks.clear();
    releases.clear();
    taps.clear();
    keys.clear();
    keyReleases.clear();
}

void Automation::At(int when, std::function<void()> action) {
    if (action) timeline.push_back({when, std::move(action)});
}

void Automation::Every(std::function<void(int)> action) {
    if (action) everyFrame.push_back(std::move(action));
}

void Automation::Click(Vector2 canvas, int button) {
    if (button >= 0 && button < 8) clicks.push_back({canvas, button});
}

void Automation::PressKey(int key) {
    if (key > 0 && key < 512) keys.push_back(key);
}

void Automation::Tap(Vector2 canvas, int touchId) { taps.push_back({canvas, touchId}); }

void Automation::BeginFrame() {
    if (!active) return;
    ++frame;
    // Copies: an action may add actions or stop the run.
    auto every = everyFrame;
    for (auto& action : every) {
        if (!active) return;
        action(frame);
    }
    std::vector<Timed> due;
    for (auto it = timeline.begin(); it != timeline.end();) {
        if (it->frame <= frame) {
            due.push_back(std::move(*it));
            it = timeline.erase(it);
        } else {
            ++it;
        }
    }
    for (auto& entry : due) {
        if (!active) return;
        entry.action();
    }
}

void Automation::Overlay(InputFrame& input) {
    input.focused = true;
    for (const auto& r : releases) {
        input.mousePosition = Display::CanvasToScreen(r.canvas);
        input.mouse.held[r.code] = false;
        input.mouse.released[r.code] = true;
    }
    releases.clear();
    for (int key : keyReleases) {
        input.keys.held[key] = false;
        input.keys.released[key] = true;
    }
    keyReleases.clear();
    for (const auto& c : clicks) {
        input.mousePosition = Display::CanvasToScreen(c.canvas);
        input.mouse.held[c.code] = true;
        input.mouse.pressed[c.code] = true;
        releases.push_back(c);
    }
    clicks.clear();
    for (int key : keys) {
        input.keys.held[key] = true;
        input.keys.pressed[key] = true;
        keyReleases.push_back(key);
    }
    keys.clear();
    for (const auto& t : taps)
        if (input.touchCount < (int)input.touches.size()) input.touches[input.touchCount++] = {t.code, Display::CanvasToScreen(t.canvas)};
    taps.clear();
}

std::filesystem::path Automation::OutputPath(const std::string& name) {
    auto path = std::filesystem::u8path(name);
    if (!path.has_extension()) path += ".png";
    return path.is_absolute() ? path : directory / path;
}

bool Automation::SaveCanvas(const std::string& name, Vector2 size) {
    Texture2D canvas = Display::GetCanvasTexture();
    if (!canvas.id || Display::InFrame()) return false;
    // The canvas texture is padded and supersampled; its logical size undoes both.
    if (size.x <= 0 || size.y <= 0) size = Display::GetSize();
    return ExportOpaque(LoadImageFromTexture(canvas), size, OutputPath(name));
}

bool Automation::SaveWindow(const std::string& name) {
    if (!Display::GetCanvasTexture().id || Display::InFrame()) return false;
    Rectangle window{0, 0, (float)::GetScreenWidth(), (float)::GetScreenHeight()};
    RenderTexture2D target = LoadRenderTexture((int)window.width, (int)window.height);
    if (!IsRenderTextureValid(target)) return false;
    Display::PreparePresentation(window);
    BeginTextureMode(target);
    Display::DrawPresentation(window);
    EndTextureMode();
    Image image = LoadImageFromTexture(target.texture);
    UnloadRenderTexture(target);
    return ExportOpaque(image, {0, 0}, OutputPath(name));
}

} // namespace RayGame
