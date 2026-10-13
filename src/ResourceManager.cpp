#include "ResourceManager.h"
#include "Diagnostics.h"
#include "FileSystem.h"
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
std::unordered_map<std::string, FontOptions> fontOptions;
void ApplyFontOptions(const Slot<Font>& slot) {
    auto it = fontOptions.find(slot->name);
    if (it == fontOptions.end() || slot->state != ResourceState::Ready || !slot->value.texture.id) return;
    if (it->second.mipmaps && slot->value.texture.mipmaps <= 1) GenTextureMipmaps(&slot->value.texture);
    if (it->second.filter >= 0) SetTextureFilter(slot->value.texture, it->second.filter);
}
Texture2D Upload(Image image) {
    if (!image.data) return {};
    auto texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}
}
void ResourceManager::Shutdown() { Clear(textures); Clear(fonts); Clear(shaders); fontOptions.clear(); }
std::vector<int> ResourceManager::CodepointsFromText(const std::string& text, bool includeAscii) {
    std::vector<int> points;
    if (includeAscii) for (int c = 32; c < 127; ++c) points.push_back(c);
    size_t i = 0;
    while (i < text.size()) {
        auto byte = (unsigned char)text[i];
        int length = byte < 0x80 ? 1 : (byte >> 5) == 0x6 ? 2 : (byte >> 4) == 0xe ? 3 : (byte >> 3) == 0x1e ? 4 : 0;
        if (length == 0 || i + length > text.size()) { ++i; continue; }
        int code = length == 1 ? byte : byte & (0x7f >> length);
        bool valid = true;
        for (int k = 1; k < length; ++k) {
            auto next = (unsigned char)text[i + k];
            if ((next & 0xc0) != 0x80) { valid = false; break; }
            code = (code << 6) | (next & 0x3f);
        }
        static const int minimum[] = {0, 0, 0x80, 0x800, 0x10000};
        if (!valid) { ++i; continue; }
        i += length;
        if (code < minimum[length] || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff)) continue;
        if (code < 32 || (code >= 127 && code < 160)) continue;
        points.push_back(code);
    }
    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());
    return points;
}
void ResourceManager::SetFontOptions(const std::string& name, const FontOptions& options) {
    fontOptions[name] = options;
    if (auto slot = Find(fonts, name)) ApplyFontOptions(slot);
}
TextureHandle ResourceManager::GetTexture(const std::string& name) { return TextureHandle(Find(textures, name)); }
FontHandle ResourceManager::GetFont(const std::string& name) { return FontHandle(Find(fonts, name)); }
ShaderHandle ResourceManager::GetShader(const std::string& name) { return ShaderHandle(Find(shaders, name)); }
void ResourceManager::UnloadTextureNamed(const std::string& name) { Unload(textures, name); }
void ResourceManager::UnloadFontNamed(const std::string& name) { Unload(fonts, name); fontOptions.erase(name); }
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
    return ReloadFont(name,path,size,{});
}
FontHandle ResourceManager::LoadFontResource(const std::string& name, const std::string& path, int size,
                                           const std::vector<int>& codepoints) {
    if (!GetFont(name)) ReloadFont(name,path,size,codepoints);
    return GetFont(name);
}
bool ResourceManager::ReloadFont(const std::string& name, const std::string& path, int size,
                                const std::vector<int>& codepoints) {
    auto slot = Ensure(fonts, name);
    auto codes = codepoints;
    bool validCodes = codes.size() <= 8192 && std::all_of(codes.begin(),codes.end(),[](int code) {
        return code > 0 && code <= 0x10ffff && (code < 0xd800 || code > 0xdfff);
    });
    Font value = IsWindowReady() && size > 0 && validCodes
        ? LoadFontEx(path.c_str(), size, codes.empty() ? nullptr : codes.data(), (int)codes.size()) : Font{};
    bool valid = value.texture.id && value.texture.id != GetFontDefault().texture.id;
    bool stored = Store(slot, value, valid, path);
    if (stored) ApplyFontOptions(slot);
    return stored;
}
ShaderHandle ResourceManager::LoadShaderResource(const std::string& name, const std::string& vs, const std::string& fs) {
    if (!GetShader(name)) ReloadShader(name, vs, fs);
    return GetShader(name);
}
bool ResourceManager::ReloadShader(const std::string& name, const std::string& vs, const std::string& fs) {
    auto slot = Ensure(shaders, name);
    bool files = (vs.empty() || FileSystem::AssetExists(vs)) && (fs.empty() || FileSystem::AssetExists(fs));
    Shader value = IsWindowReady() && files && (!vs.empty() || !fs.empty())
        ? ::LoadShader(vs.empty() ? nullptr : vs.c_str(), fs.empty() ? nullptr : fs.c_str()) : Shader{};
    bool valid = value.id && value.id != rlGetShaderIdDefault();
    return Store(slot, value, valid, "vertex=" + vs + ", fragment=" + fs);
}
}
