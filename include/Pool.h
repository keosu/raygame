#pragma once

#include "Core.h"
#include <unordered_set>

namespace RayGame {

// Generic free-list object pool. T must be default-constructible.
// Acquire() returns a pointer owned by the pool; Release() returns it for reuse.
template<typename T>
class ObjectPool {
public:
    explicit ObjectPool(size_t initial = 0) {
        for (size_t i = 0; i < initial; i++) {
            storage.push_back(std::make_unique<T>());
            freeList.push_back(storage.back().get());
            available.insert(storage.back().get());
            owned.insert(storage.back().get());
        }
    }

    T* Acquire() {
        if (freeList.empty()) {
            storage.push_back(std::make_unique<T>());
            owned.insert(storage.back().get());
            return storage.back().get();
        }
        T* obj = freeList.back();
        freeList.pop_back();
        available.erase(obj);
        return obj;
    }

    void Release(T* obj) {
        if (!obj || available.count(obj)) return;
        if (!owned.count(obj)) return;
        freeList.push_back(obj);
        available.insert(obj);
    }

    void Reset() {
        freeList.clear();
        available.clear();
        for (auto& ptr : storage) { freeList.push_back(ptr.get()); available.insert(ptr.get()); }
    }

    size_t Capacity() const { return storage.size(); }
    size_t Available() const { return freeList.size(); }
    size_t InUse() const { return storage.size() - freeList.size(); }

private:
    std::vector<std::unique_ptr<T>> storage;
    std::vector<T*> freeList;
    std::unordered_set<T*> available;
    std::unordered_set<T*> owned;
};

}
