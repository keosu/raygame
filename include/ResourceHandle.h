#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <cstdint>
#include <utility>
#include "raylib.h"

namespace RayGame {
enum class ResourceState { Empty, Ready, Failed, Unloaded };
namespace detail {
template<class T> struct ResourceSlot {
    T value{};
    ResourceState state = ResourceState::Empty;
    std::string name, error;
    std::uint64_t revision = 0;
};
}
// Keeps metadata alive, never the GPU allocation. Read and mutate on the
// graphics thread. Get() borrows a pointer until the next manager mutation.
template<class T> class ResourceHandle {
public:
    ResourceHandle() = default;
    ResourceHandle(std::nullptr_t) {}
    explicit operator bool() const { return Get() != nullptr; }
    const T* Get() const {
        return slot && slot->state == ResourceState::Ready ? &slot->value : nullptr;
    }
    const T* operator->() const {
        auto* value = Get();
        if (!value) throw std::logic_error("Invalid resource handle: " + Name());
        return value;
    }
    const T& operator*() const { return *operator->(); }
    ResourceState State() const { return slot ? slot->state : ResourceState::Empty; }
    std::string Name() const { return slot ? slot->name : std::string{}; }
    std::string Error() const { return slot ? slot->error : std::string{}; }
    std::uint64_t Revision() const { return slot ? slot->revision : 0; }
    bool operator==(const ResourceHandle& other) const { return slot == other.slot; }
    bool operator!=(const ResourceHandle& other) const { return !(*this == other); }
    bool operator==(std::nullptr_t) const { return !Get(); }
    bool operator!=(std::nullptr_t) const { return Get() != nullptr; }
private:
    friend class ResourceManager;
    explicit ResourceHandle(std::shared_ptr<detail::ResourceSlot<T>> value) : slot(std::move(value)) {}
    std::shared_ptr<detail::ResourceSlot<T>> slot;
};
using TextureHandle = ResourceHandle<Texture2D>;
using FontHandle = ResourceHandle<Font>;
using ShaderHandle = ResourceHandle<Shader>;
}
