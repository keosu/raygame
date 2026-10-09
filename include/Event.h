#pragma once

#include "Core.h"

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

    static unsigned long Subscribe(const std::string& event, Handler handler) {
        unsigned long id = ++nextId;
        handlers[event].push_back({id, std::move(handler)});
        return id;
    }

    static void Unsubscribe(const std::string& event, unsigned long id) {
        auto it = handlers.find(event);
        if (it == handlers.end()) return;
        auto& list = it->second;
        list.erase(std::remove_if(list.begin(), list.end(),
            [id](const Entry& e) { return e.id == id; }), list.end());
    }

    static void Emit(const std::string& event, const EventData& data = {}) {
        auto it = handlers.find(event);
        if (it == handlers.end()) return;
        // Copy so handlers may subscribe/unsubscribe during dispatch.
        auto listeners = it->second;
        for (auto& e : listeners) {
            auto live = handlers.find(event);
            if (live == handlers.end()) break;
            bool subscribed = std::any_of(live->second.begin(), live->second.end(),
                [&](const Entry& entry) { return entry.id == e.id; });
            if (subscribed && e.handler) e.handler(data);
        }
    }

    static void Clear() { handlers.clear(); }

private:
    struct Entry { unsigned long id; Handler handler; };
    static inline std::unordered_map<std::string, std::vector<Entry>> handlers;
    static inline unsigned long nextId = 0;
};

}
