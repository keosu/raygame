#pragma once

#include "Core.h"
#include "CallbackScope.h"

namespace RayGame {

// Lightweight string-keyed event bus with an optional integer/float/pointer payload.
struct EventData {
    int i = 0;
    float f = 0;
    void* ptr = nullptr;
    Vector2 vec = {0, 0};
};

class RAYGAME_API EventBus {
public:
    using Handler = std::function<void(const EventData&)>;

    static unsigned long Subscribe(const std::string& event, Handler handler,
                                   ScopeToken owner = ScopeToken::Current()) {
        unsigned long id = ++nextId;
        if (!owner.IsAlive() || !handler) return id;
        auto entry = std::make_shared<Entry>();
        entry->id = id;
        entry->owner = owner;
        entry->handler = std::make_shared<Handler>(std::move(handler));
        entry->cancellation = owner.OnCancel([event, id] { Unsubscribe(event, id); });
        auto& list = handlers[event];
        list.erase(std::remove_if(list.begin(), list.end(),
            [](const auto& value) { return !value->handler; }), list.end());
        list.push_back(entry);
        return id;
    }

    static void Unsubscribe(const std::string& event, unsigned long id) {
        auto it = handlers.find(event);
        if (it == handlers.end()) return;
        auto& list = it->second;
        auto found = std::find_if(list.begin(), list.end(), [id](const auto& e) { return e->id == id; });
        if (found == list.end()) return;
        auto removed = std::move(*found);
        list.erase(found);
        if (list.empty()) handlers.erase(it);
        // Detach first: destroying captured values can itself cancel an owner.
        removed->cancellation.Reset();
        removed->handler.reset();
    }

    static void Emit(const std::string& event, const EventData& data = {}) {
        auto it = handlers.find(event);
        if (it == handlers.end()) return;
        // Copy so handlers may subscribe/unsubscribe during dispatch.
        auto listeners = it->second;
        for (auto& e : listeners) {
            // Retain the callable itself during self-unsubscribe/owner destruction.
            auto callback = e->handler;
            if (callback && e->owner.IsAlive()) {
                ScopeActivation scope(e->owner);
                (*callback)(data);
            }
        }
    }

    static void Clear() {
        auto removed = std::move(handlers);
        handlers.clear();
        for (auto& list : removed) for (auto& entry : list.second) {
            entry->cancellation.Reset(); entry->handler.reset();
        }
    }

private:
    struct Entry {
        unsigned long id = 0;
        std::shared_ptr<Handler> handler;
        ScopeToken owner;
        CancellationLink cancellation;
    };
    static inline std::unordered_map<std::string, std::vector<std::shared_ptr<Entry>>> handlers;
    static inline unsigned long nextId = 0;
};

}
