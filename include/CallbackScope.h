#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

namespace RayGame {
namespace detail {
struct CallbackState {
    bool alive = true;
    unsigned long nextId = 0;
    std::unordered_map<unsigned long, std::function<void()>> cancellations;
    void Cancel() noexcept {
        if (!alive) return;
        alive = false;
        auto callbacks = std::move(cancellations);
        for (auto& entry : callbacks) {
            try { entry.second(); } catch (...) {} // Cancellation must finish all owners.
        }
    }
};
}

// Removes its cancellation hook when the registration itself finishes.
class CancellationLink {
public:
    CancellationLink() = default;
    CancellationLink(std::weak_ptr<detail::CallbackState> state, unsigned long id) : state(state), id(id) {}
    CancellationLink(const CancellationLink&) = delete;
    CancellationLink& operator=(const CancellationLink&) = delete;
    CancellationLink(CancellationLink&& other) noexcept : state(std::move(other.state)), id(other.id) {}
    CancellationLink& operator=(CancellationLink&& other) noexcept {
        if (this != &other) { Reset(); state = std::move(other.state); id = other.id; }
        return *this;
    }
    ~CancellationLink() { Reset(); }
    void Reset() { if (auto owner = state.lock()) owner->cancellations.erase(id); state.reset(); }
private:
    std::weak_ptr<detail::CallbackState> state;
    unsigned long id = 0;
};

class ScopeToken {
public:
    // An empty token is application/global lifetime, until service shutdown.
    ScopeToken() = default;
    bool IsAlive() const { auto value = state.lock(); return !bound || (value && value->alive); }
    CancellationLink OnCancel(std::function<void()> callback) const {
        if (!bound) return {};
        auto value = state.lock();
        if (!value || !value->alive) { callback(); return {}; }
        auto id = ++value->nextId;
        value->cancellations.emplace(id, std::move(callback));
        return {value, id};
    }
    static ScopeToken Current() { return current; }
private:
    friend class CallbackScope;
    friend class ScopeActivation;
    explicit ScopeToken(const std::shared_ptr<detail::CallbackState>& value) : state(value), bound(true) {}
    std::weak_ptr<detail::CallbackState> state;
    bool bound = false;
    static thread_local ScopeToken current;
};
inline thread_local ScopeToken ScopeToken::current;

class ScopeActivation {
public:
    explicit ScopeActivation(ScopeToken token) : previous(ScopeToken::current) { ScopeToken::current = token; }
    ~ScopeActivation() { ScopeToken::current = previous; }
    ScopeActivation(const ScopeActivation&) = delete;
    ScopeActivation& operator=(const ScopeActivation&) = delete;
private:
    ScopeToken previous;
};

// Explicit lifetime owner; all APIs here are confined to the runtime thread.
class CallbackScope {
public:
    explicit CallbackScope(ScopeToken parent = {}) { SetParent(parent); }
    ~CallbackScope() { Cancel(); }
    CallbackScope(const CallbackScope&) = delete;
    CallbackScope& operator=(const CallbackScope&) = delete;
    ScopeToken Token() const { return ScopeToken(state); }
    void Cancel() noexcept { state->Cancel(); parentLink.Reset(); }
    void SetParent(ScopeToken parent) {
        parentLink.Reset();
        std::weak_ptr<detail::CallbackState> weak = state;
        parentLink = parent.OnCancel([weak] { if (auto value = weak.lock()) value->Cancel(); });
    }
    void Renew(ScopeToken parent = {}) {
        Cancel();
        state = std::make_shared<detail::CallbackState>();
        SetParent(parent);
    }
private:
    std::shared_ptr<detail::CallbackState> state = std::make_shared<detail::CallbackState>();
    CancellationLink parentLink;
};
}
