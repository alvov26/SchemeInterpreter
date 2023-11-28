#include <scheme/scheme.h>
#include <scheme/tokenizer.h>
#include <scheme/parser.h>

#include <scheme/heap.h>
#include <scheme/error.h>

#include <sstream>


std::string Interpreter::Run(const std::string &str) {
    std::stringstream ss{str};
    Tokenizer tokenizer(&ss);

    auto ast = Read(&tokenizer);
    auto eval = Eval(ast, global_scope_);
    std::string result = ToString(eval);
    Heap::Instance().Mark(global_scope_);
    Heap::Instance().Sweep();
    return result;
}

ArgList::ArgList(Object* ast) {
    is_proper_ = true;
    while (ast) {
        if (Is<Cell>(ast)) {
            vec_.push_back(As<Cell>(ast)->GetFirst());
            ast = As<Cell>(ast)->GetSecond();
            continue;
        }
        vec_.push_back(ast);
        is_proper_ = false;
        ast = nullptr;
    }
}

ArgList& ArgList::ExpectSize(size_t size) {
    if (vec_.size() != size) {
        throw RuntimeError("Invalid function call");
    }
    return *this;
}

ArgList& ArgList::ExpectSizeAtLeast(size_t size) {
    if (vec_.size() < size) {
        throw RuntimeError("Invalid function call");
    }
    return *this;
}

std::string ArgList::ToString() {
    std::string result = "(";
    for (size_t i = 0; i < vec_.size(); ++i) {
        auto el = At(i);
        if (i + 1 == vec_.size() && !is_proper_) {
            result += ". ";
        }
        result += ::ToString(el) + " ";
    }
    if (result != "(") {
        result.pop_back();
    }
    result += ")";
    return result;
}

bool ArgList::IsProper() const {
    return is_proper_;
}
Object* ArgList::At(size_t i) const {
    if (i >= vec_.size()) {
        throw RuntimeError("Too few arguments");
    }
    return vec_[i];
}


