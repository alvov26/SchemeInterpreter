//
// Created by Aleksandr Lvov on 19/11/2023.
//

#pragma once

#include <set>
#include <string>
#include <string_view>
#include <iostream>
#include "object.h"

class Heap {
    std::vector<std::unique_ptr<Object>> objects_;

private:
    Heap() = default;
    Heap(const Heap&) = delete;
    Heap(Heap&&) = delete;

public:
    static Heap& Instance() {
        static Heap heap;
        return heap;
    }

public:
    template <std::derived_from<Object> T, class... Args>
    T* Make(Args... args) requires std::constructible_from<T, Args...> {
        std::unique_ptr<T> ptr = std::make_unique<T>(args...);
        T* raw_ptr = ptr.get();
        objects_.push_back(std::move(ptr));
        return raw_ptr;
    };

    void Mark(Object* o) {
        if (o) {
            o->Mark();
        }
    }

    void Sweep() {
        std::erase_if(objects_, [](auto& o) { return not o->is_reachable_; });
        for (auto& o : objects_) {
            o->is_reachable_ = false;
        }
    }
};


