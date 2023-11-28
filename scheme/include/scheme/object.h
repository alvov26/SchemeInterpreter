#pragma once

#include <map>
#include <string>
#include <memory>
#include "error.h"

enum class ShouldEval {
    Yes, No
};

class Environment;

class Object {
    bool is_reachable_ = false;

    void Mark();

protected:
    virtual Object* Eval(Environment*) = 0;
    virtual std::string ToString() const = 0;
    virtual void MarkDependencies();

public:
    friend Object* Eval(Object* ast, Environment* scope);
    friend std::string ToString(Object* ast);
    friend class Heap;

public:
    virtual ~Object() = default;
};

class Number : public Object {
    int64_t value_;

public:
    explicit Number(int64_t value);
    int64_t GetValue() const;

protected:
    Object* Eval(Environment*) override;
    std::string ToString() const override;
};

class Symbol : public Object {
    const std::string name_;

public:
    static Object* True();
    static Object* False();

    explicit Symbol(std::string name);
    const std::string& GetName() const;

protected:
    Object* Eval(Environment*) override;
    std::string ToString() const override;
};

class Cell : public Object {
    Object* first_;
    Object* second_;

public:
    Cell(Object* first, Object* second);
    Object* GetFirst() const;
    Object* GetSecond() const;

    void SetFirst(Object*);
    void SetSecond(Object*);

protected:
    void MarkDependencies() override;
    Object* Eval(Environment*) override;
    std::string ToString() const override;
};

class Callable : public Object {
public:
    virtual Object* Call(Object* ast, Environment* env) = 0;
};

class BuiltInSyntax : public Callable {
    std::function<Object*(Object*, Environment*)> value_;

public:
    BuiltInSyntax(std::function<Object*(Object*, Environment*)> value);
    Object* Call(Object*, Environment*) override;

protected:
    Object* Eval(Environment*) override;
    std::string ToString() const override;
};

template <std::derived_from<Object> T = Object>
class BuiltInProc : public Callable {
    std::function<Object*(const std::vector<T*>&)> value_;

public:
    BuiltInProc(std::function<Object*(const std::vector<T*>&)> value) : value_(value) {}
    Object* Call(Object* o, Environment* s) override {
        return value_(AsVector<T, ShouldEval::Yes>(o, s));
    }

protected:
    Object* Eval(Environment*) override {
        throw RuntimeError("Trying to evaluate a procedure");
    }
    std::string ToString() const override {
        return "BuiltInProcedure";
    }
};

class Lambda : public Callable {
    Object* ast_;
    std::vector<Symbol*> formals_;
    Environment* parent_scope_;

public:
    Lambda(std::vector<Symbol*> formals, Object* ast, Environment*);
    Object* Call(Object*, Environment*) override;

protected:
    void MarkDependencies() override;
    Object* Eval(Environment*) override;
    std::string ToString() const override;
};

class Environment : public Object {
    std::map<std::string, Object*> names_;
    Environment* parent_ = nullptr;

public:
    static Environment* R5RS();

    Object* GetDefinition(const std::string&);
    void NewDefinition(const std::string&, Object*);
    void SetDefinition(const std::string&, Object*);

    void SetParent(Environment*);

protected:
    void MarkDependencies() override;
    Object* Eval(Environment* ptr) override;
    std::string ToString() const override;
};

///////////////////////////////////////////////////////////////////////////////

Object* Eval(Object* ast, Environment* scope);
std::string ToString(Object* ast);

///////////////////////////////////////////////////////////////////////////////

// Runtime type checking and convertion.
// This can be helpful: https://en.cppreference.com/w/cpp/memory/shared_ptr/pointer_cast

template <std::derived_from<Object> T>
T* As(Object* obj)  {
    if constexpr (std::is_same_v<T, Object>) {
        return obj;
    }
    auto ptr = dynamic_cast<T*>(obj);
    if (ptr == nullptr) {
        throw RuntimeError("Expected type does not match.");
    }
    return ptr;
}

template <std::derived_from<Object> T>
bool Is(Object* obj) {
    if constexpr (std::is_same_v<T, Object>) {
        return true;
    }
    return dynamic_cast<T*>(obj) != nullptr;
}

template <size_t N, std::derived_from<Object> T>
inline void RequireSize(const std::vector<T*>& v) {
    if (v.size() != N) {
        throw RuntimeError("Invalid function call.");
    }
}

template <size_t N, std::derived_from<Object> T>
inline void RequireSizeAtLeast(const std::vector<T*>& v) {
    if (v.size() < N) {
        throw RuntimeError("Invalid function call.");
    }
}

template <std::derived_from<Object> T = Object, ShouldEval shouldEval = ShouldEval::No>
auto ListIterator(Object* args, Environment* env) {
    struct Wrapper {
        Object* start;
        Environment* env;
        bool is_proper = true;
        auto begin() { return Iterator{ start, env, this }; } // NOLINT(*-identifier-naming)
        auto end() { return Iterator{ nullptr, env, this }; } // NOLINT(*-identifier-naming)

        struct Iterator {
            Object* o;
            Environment* env;
            Wrapper* wrapper;
            Iterator& operator++(){
                if (Is<Cell>(o)) {
                    o = As<Cell>(o)->GetSecond();
                    if (o == nullptr) {
                        wrapper->is_proper = false;
                    }
                } else {
                    o = nullptr;
                }
                return *this;
            }
            bool operator!=(Iterator other) { return o != other.o; }
            T* operator*() const {
                if constexpr (shouldEval == ShouldEval::Yes) {
                    if (Is<Cell>(o)) {
                        return As<T>(::Eval(As<Cell>(o)->GetFirst(), env));
                    }
                    return As<T>(::Eval(o, env));
                } else {
                    if (Is<Cell>(o)) {
                        return As<T>(As<Cell>(o)->GetFirst());
                    }
                    return As<T>(o);
                }
            }
        };
    };
    return Wrapper{args, env};
}

template <std::derived_from<Object> T, ShouldEval shouldEval>
std::vector<T*> AsVector(Object* o, Environment* env) {
    std::vector<T*> vec;
    for (auto it : ListIterator<T, shouldEval>(o, env)) {
        vec.push_back(it);
    }
    return vec;
}

template <std::derived_from<Object> T = Object, ShouldEval shouldEval = ShouldEval::No>
auto ListIteratorF(Object* args, Environment* env) {
    struct Wrapper {
        Object* start;
        Environment* env;
        bool is_proper = true;
        auto begin() { return Iterator{ start, env, this }; } // NOLINT(*-identifier-naming)
        auto end() { return Iterator{ nullptr, env, this }; } // NOLINT(*-identifier-naming)

        struct Iterator {
            Object* o;
            Environment* env;
            Wrapper* wrapper;
            Iterator& operator++(){
                if (Is<Cell>(o)) {
                    o = As<Cell>(o)->GetSecond();
                    if (o == nullptr) {
                        wrapper->is_proper = false;
                    }
                } else {
                    o = nullptr;
                }
                return *this;
            }
            bool operator!=(Iterator other) { return o != other.o; }
            T* operator*() const {
                if constexpr (shouldEval == ShouldEval::Yes) {
                    if (Is<Cell>(o)) {
                        return As<T>(::Eval(As<Cell>(o)->GetFirst(), env));
                    }
                    return As<T>(::Eval(o, env));
                } else {
                    if (Is<Cell>(o)) {
                        return As<T>(As<Cell>(o)->GetFirst());
                    }
                    return As<T>(o);
                }
            }
        };
    };
    return Wrapper{args, env};
}