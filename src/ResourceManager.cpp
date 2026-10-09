#include "ResourceManager.h"
#include "Diagnostics.h"
#include "rlgl.h"
#include <limits>

namespace RayGame {
namespace {
template<class T> using Slot = std::shared_ptr<detail::ResourceSlot<T>>;
template<class T> using Cache = std::unordered_map<std::string, Slot<T>>;
Cache<Texture2D> textures;
Cache<Font> fonts;
Cache<Shader> shaders;
template<class T> Slot<T> Find(Cache<T>& cache, const std::string& name) {
    auto it = cache.find(name);
    return it == cache.end() ? nullptr : it->second;
}
template<class T> Slot<T> Ensure(Cache<T>& cache, const std::string& name) {
    auto& slot = cache[name];
    if (!slot) { slot = std::make_shared<detail::ResourceSlot<T>>(); slot->name = name; }
    return slot;
}
void Release(Texture2D value) { ::UnloadTexture(value); }
void Release(Font value) { ::UnloadFont(value); }
void Release(Shader value) { ::UnloadShader(value); }
template<class T> bool Store(const Slot<T>& slot, T value, bool valid, const std::string& source) {
    if (!valid) {
        slot->error = "Failed to load resource '" + slot->name + "': " + source;
        if (slot->state != ResourceState::Ready) slot->state = ResourceState::Failed;
        Diagnostics::Write(LogLevel::Error, slot->error);
        return false;
    }
    if (slot->state == ResourceState::Ready) Release(slot->value);
    slot->value = value;
    slot->state = ResourceState::Ready;
    slot->error.clear();
    ++slot->revision;
    return true;
}
template<class T> void Invalidate(const Slot<T>& slot) {
    if (slot->state == ResourceState::Ready) Release(slot->value);
    slot->value = {};
    slot->state = ResourceState::Unloaded;
    ++slot->revision;
}
template<class T> void Unload(Cache<T>& cache, const std::string& name) {
    auto it = cache.find(name);
    if (it == cache.end()) return;
    Invalidate(it->second);
    cache.erase(it); // Never reuse an invalidated slot.
}
template<class T> void Clear(Cache<T>& cache) {
    for (auto& entry : cache) Invalidate(entry.second);
    cache.clear();
}
Texture2D Upload(Image image) {
    if (!image.data) return {};
    auto texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}
}
void ResourceManager::Shutdown() { Clear(textures); Clear(fonts); Clear(shaders); }
TextureHandle ResourceManager::GetTexture(const std::string& name) { return TextureHandle(Find(textures, name)); }
FontHandle ResourceManager::GetFont(const std::string& name) { return FontHandle(Find(fonts, name)); }
ShaderHandle ResourceManager::GetShader(const std::string& name) { return ShaderHandle(Find(shaders, name)); }
void ResourceManager::UnloadTextureNamed(const std::string& name) { Unload(textures, name); }
void ResourceManager::UnloadFontNamed(const std::string& name) { Unload(fonts, name); }
void ResourceManager::UnloadShaderNamed(const std::string& name) { Unload(shaders, name); }
TextureHandle ResourceManager::LoadTexture(const std::string& name, const std::string& path) {
    if (!GetTexture(name)) ReloadTexture(name, path);
    return GetTexture(name);
}
bool ResourceManager::ReloadTexture(const std::string& name, const std::string& path) {
    auto slot = Ensure(textures, name);
    Texture2D value = IsWindowReady() ? ::LoadTexture(path.c_str()) : Texture2D{};
    return Store(slot, value, value.id != 0, path);
}
TextureHandle ResourceManager::CreateColorTexture(const std::string& name, int width, int height, Color color) {
    if (auto cached = GetTexture(name)) return cached;
    auto slot = Ensure(textures, name);
    Texture2D value{};
    if (IsWindowReady() && width > 0 && height > 0) value = Upload(GenImageColor(width, height, color));
    Store(slot, value, value.id != 0, "generated color texture");
    return TextureHandle(slot);
}
TextureHandle ResourceManager::CreateCircleTexture(const std::string& name, int radius, Color color) {
    if (auto cached = GetTexture(name)) return cached;
    auto slot = Ensure(textures, name);
    Texture2D value{};
    if (IsWindowReady() && radius > 0 && radius <= std::numeric_limits<int>::max() / 2) {
        int size = radius * 2;
        Image img = GenImageColor(size, size, BLANK);
        if (img.data) for (int y = 0; y < size; ++y) for (int x = 0; x < size; ++x) {
            float dx = x - radius + 0.5f, dy = y - radius + 0.5f;
            float coverage = Clamp(radius + 0.5f - std::sqrt(dx*dx + dy*dy), 0.0f, 1.0f);
            Color pixel = color;
            pixel.a = (unsigned char)(color.a * coverage);
            if (coverage > 0) ImageDrawPixel(&img, x, y, pixel);
        }
        value = Upload(img);
        if (value.id) SetTextureFilter(value, TEXTURE_FILTER_BILINEAR);
    }
    Store(slot, value, value.id != 0, "generated circle texture");
    return TextureHandle(slot);
}
FontHandle ResourceManager::LoadFontResource(const std::string& name, const std::string& path, int size) {
    if (!GetFont(name)) ReloadFont(name, path, size);
    return GetFont(name);
}
bool ResourceManager::ReloadFont(const std::string& name, const std::string& path, int size) {
    auto slot = Ensure(fonts, name);
    Font value = IsWindowReady() && size > 0 ? LoadFontEx(path.c_str(), size, nullptr, 0) : Font{};
    bool valid = value.texture.id && value.texture.id != GetFontDefault().texture.id;
    return Store(slot, value, valid, path);
}
ShaderHandle ResourceManager::LoadShaderResource(const std::string& name, const std::string& vs, const std::string& fs) {
    if (!GetShader(name)) ReloadShader(name, vs, fs);
    return GetShader(name);
}
bool ResourceManager::ReloadShader(const std::string& name, const std::string& vs, const std::string& fs) {
    auto slot = Ensure(shaders, name);
    bool files = (vs.empty() || FileExists(vs.c_str())) && (fs.empty() || FileExists(fs.c_str()));
    Shader value = IsWindowReady() && files && (!vs.empty() || !fs.empty())
        ? ::LoadShader(vs.empty() ? nullptr : vs.c_str(), fs.empty() ? nullptr : fs.c_str()) : Shader{};
    bool valid = value.id && value.id != rlGetShaderIdDefault();
    return Store(slot, value, valid, "vertex=" + vs + ", fragment=" + fs);
}
}
