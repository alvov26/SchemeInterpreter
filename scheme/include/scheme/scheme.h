#pragma once

#include <map>
#include <string>
#include "object.h"
#include "heap.h"

class ArgList {
    std::vector<Object*> vec_;
    bool is_proper_;

public:
    explicit ArgList(Object* ast);

    bool IsProper() const;
    Object* At(size_t i) const;

    template <class T = Object>
    T* EvalAs(size_t i, Environment* scope) {
        return As<T>(Eval(i, scope));
    }

    Object* Eval(size_t i, Environment* scope) {
        return ::Eval(At(i), scope);
    }

    ArgList& ExpectSize(size_t size);
    ArgList& ExpectSizeAtLeast(size_t size);
    std::string ToString();

    auto Size() { return vec_.size(); }
};

class Interpreter {
    Environment* global_scope_ = Environment::R5RS();
public:
    std::string Run(const std::string&);
};
