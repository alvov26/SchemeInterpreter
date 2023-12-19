//
// Created by Aleksandr Lvov on 12/11/2023.
//
#include <scheme/object.h>
#include <scheme/heap.h>
#include <scheme/error.h>
#include <scheme/scheme.h>

#include <numeric>

Object* Eval(Object* ast, Environment* scope) {
    if (ast == nullptr) {
        throw RuntimeError("() cannot be evaluated");
    }
    return ast->Eval(scope);
}

std::string ToString(Object* ast) {
    if (ast == nullptr) {
        return "()";
    }
    return ast->ToString();
}
void Object::Mark() {
    if (not is_reachable_) {
        is_reachable_ = true;
        MarkDependencies();
    }
}
[[maybe_unused]] void Object::MarkDependencies() {}

Number::Number(int64_t value) : value_(value) {}
int64_t Number::GetValue() const { return value_; }
Object* Number::Eval(Environment*) { return this; }
std::string Number::ToString() const { return std::to_string(value_); }

Object* Symbol::True()  { return Heap::Instance().Make<Symbol>("#t"); }
Object* Symbol::False() { return Heap::Instance().Make<Symbol>("#f"); }

Symbol::Symbol(std::string name) : name_(std::move(name)) {}
const std::string& Symbol::GetName() const { return name_; }
Object* Symbol::Eval(Environment* scope) { return scope->GetDefinition(name_); }
std::string Symbol::ToString() const { return name_; }

Cell::Cell(Object* first, Object* second) : first_(first), second_(second) {}
Object* Cell::GetFirst() const { return first_; }
Object* Cell::GetSecond() const { return second_; }
Object* Cell::Eval(Environment* scope) {
    return As<Callable>(::Eval(first_, scope))->Call(second_, scope);
}
std::string Cell::ToString() const {
    return ArgList((Object*)this).ToString();
}
void Cell::SetFirst(Object* o) { first_ = o; }
void Cell::SetSecond(Object* o) { second_ = o; }
void Cell::MarkDependencies() {
    Heap::Instance().Mark(first_);
    Heap::Instance().Mark(second_);
}

BuiltInSyntax::BuiltInSyntax(std::function<Object*(Object*, Environment*)> value) : value_(value){}

Object* BuiltInSyntax::Call(Object* o, Environment* env) {
    return value_(o, env);
}

Object* BuiltInSyntax::Eval(Environment*) {
    throw RuntimeError("Trying to evaluate a syntax keyword");
}
std::string BuiltInSyntax::ToString() const { return "BuiltInSyntax"; }

Object* BuiltInSyntaxTailRecursive::Call(Object* o, Environment* s) {
    return ::Eval(CallUntilTail(o, s), s);
}

Object* BuiltInSyntaxTailRecursive::CallUntilTail(Object* o, Environment* s) {
    return BuiltInSyntax::Call(o, s);
}

Lambda::Lambda(std::vector<Symbol*> formals, Object* ast, Environment* parent_scope)
    : ast_(ast), formals_(formals), parent_scope_(parent_scope) {}
Object* Lambda::Eval(Environment*) {
    throw SyntaxError("Trying to evaluate a procedure");
}
std::string Lambda::ToString() const {
    return "Lambda";
}
Object* Lambda::Call(Object* ast, Environment* scope) {
    while (true) {
        auto args = ArgList(ast).ExpectSize(formals_.size());
        auto local_scope = Heap::Instance().Make<Environment>();
        local_scope->SetParent(parent_scope_);
        for (size_t i = 0; i < args.Size(); ++i) {
            local_scope->NewDefinition(formals_[i]->GetName(), args.Eval(i, scope));
        }
        if (Is<Cell>(ast_)) {
            auto c = As<Cell>(ast_);
            auto cf = scope->GetDefinition(As<Symbol>(c->GetFirst())->GetName());
            if (cf == this) {
                ast = c->GetSecond();
                continue;
            }
            if (Is<BuiltInSyntaxTailRecursive>(cf)) {
                auto b = As<BuiltInSyntaxTailRecursive>(cf);
                auto tail = b->CallUntilTail(c->GetSecond(), local_scope);
                if (Is<Cell>(tail)) {
                    auto t = As<Cell>(tail);
                    if (local_scope->GetDefinition(As<Symbol>(t->GetFirst())->GetName()) == this) {
                        ast = t->GetSecond();
                        continue;
                    }
                }
                return ::Eval(tail, local_scope);
            }
        }
        return ::Eval(ast_, local_scope);
    }
}
void Lambda::MarkDependencies() {
    Heap::Instance().Mark(ast_);
    Heap::Instance().Mark(parent_scope_);
    for (auto f : formals_) {
        Heap::Instance().Mark(f);
    }
}

Object* BoolSymbol(bool b) {
    return (b ? Symbol::True() : Symbol::False());
}

bool EvalToTrue(Object* obj) {
    if (Is<Symbol>(obj)) {
        const auto& val = As<Symbol>(obj)->GetName();
        if (val == "#f") {
            return false;
        }
    }
    return true;
}

Environment* Environment::R5RS() {
    Heap& h = Heap::Instance();
    Environment* scope = h.Make<Environment>();
    auto& names = scope->names_;
    names["#t"] = Symbol::True();
    names["#f"] = Symbol::False();
    names["null?"] = h.Make<BuiltInProc<Object>>([](auto& args) {
        RequireSize<1>(args);
        return BoolSymbol(args[0] == nullptr);
    });

    names["pair?"] = h.Make<BuiltInProc<Object>>([](auto& args) {
        RequireSize<1>(args);
        return BoolSymbol(Is<Cell>(args[0]));
    });

    names["list?"] = h.Make<BuiltInProc<Object>>([](auto& args) {
        RequireSize<1>(args);
        if (args[0] != nullptr && not Is<Cell>(args[0])) {
            return Symbol::False();
        }
        if (ArgList(args[0]).IsProper()) {
            return Symbol::True();
        }
        return Symbol::False();
    });

    names["number?"] = h.Make<BuiltInProc<Object>>([](auto& args) {
        RequireSize<1>(args);
        return BoolSymbol(Is<Number>(args[0]));
    });

    names["symbol?"] = h.Make<BuiltInProc<Object>>([](auto& args) {
        RequireSize<1>(args);
        return BoolSymbol(Is<Symbol>(args[0]));
    });

    names["boolean?"] = h.Make<BuiltInProc<Object>>([](auto& args) {
        RequireSize<1>(args);
        if (not Is<Symbol>(args[0])) {
            return Symbol::False();
        }
        return BoolSymbol(As<Symbol>(args[0])->GetName() == "#t" ||
                          As<Symbol>(args[0])->GetName() == "#f");
    });

    names["not"] = h.Make<BuiltInProc<Object>>([](auto& args) {
        RequireSize<1>(args);
        return BoolSymbol(not EvalToTrue(args[0]));
    });

    names["and"] = h.Make<BuiltInSyntaxTailRecursive>([](auto ast, auto scope){
        auto args = ArgList(ast);
        if (args.Size() == 0) {
            return Symbol::True();
        }
        for (size_t i = 0; i < args.Size()-1; ++i) {
            auto eval = args.Eval(i, scope);
            if (!EvalToTrue(eval)) {
                return eval;
            }
        }
        return args.At(args.Size()-1);
    });

    names["or"] = h.Make<BuiltInSyntaxTailRecursive>([](auto ast, auto scope){
        auto args = ArgList(ast);
        if (args.Size() == 0) {
            return Symbol::False();
        }
        for (size_t i = 0; i < args.Size()-1; ++i) {
            auto eval = args.Eval(i, scope);
            if (EvalToTrue(eval)) {
                return eval;
            }
        }
        return args.At(args.Size()-1);
    });

    names["+"] = h.Make<BuiltInProc<Number>>([](auto& args) {
        int64_t value = 0;
        for (auto o : args) {
            value += o->GetValue();
        }
        return Heap::Instance().Make<Number>(value);
    });

    names["*"] = h.Make<BuiltInProc<Number>>([](auto& args) {
        int64_t value = 1;
        for (auto o : args) {
            value *= o->GetValue();
        }
        return Heap::Instance().Make<Number>(value);
    });

    names["-"] = h.Make<BuiltInProc<Number>>([](auto& args){
        RequireSizeAtLeast<1>(args);
        int64_t value = args[0]->GetValue();
        if (args.size() == 1) {
            return Heap::Instance().Make<Number>(-value);
        }
        for (size_t i = 1; i < args.size(); ++i) {
            value -= args[i]->GetValue();
        }
        return Heap::Instance().Make<Number>(value);
    });

    names["/"] = h.Make<BuiltInProc<Number>>([](auto& args){
        RequireSizeAtLeast<1>(args);
        int64_t value = args[0]->GetValue();
        if (args.size() == 1) {
            return Heap::Instance().Make<Number>(1 / value);
        }
        for (size_t i = 1; i < args.size(); ++i) {
            value /= args[i]->GetValue();
        }
        return Heap::Instance().Make<Number>(value);
    });

    names["quote"] = h.Make<BuiltInSyntax>([](auto ast, [[maybe_unused]] auto scope){
        auto args = ArgList(ast).ExpectSize(1);
        return args.At(0);
    });

    names["abs"] = h.Make<BuiltInProc<Number>>([](auto& args){
        RequireSize<1>(args);
        return Heap::Instance().Make<Number>(std::abs(args[0]->GetValue()));
    });

    names["="] = h.Make<BuiltInProc<Number>>([](auto& args){
        for (size_t i = 0; i+1 < args.size(); ++i) {
            if (args[i]->GetValue() != args[i + 1]->GetValue()) {
                return Symbol::False();
            }
        }
        return Symbol::True();
    });

    names["<"] = h.Make<BuiltInProc<Number>>([](auto& args){
        for (size_t i = 0; i+1 < args.size(); ++i) {
            if (args[i]->GetValue() >= args[i + 1]->GetValue()) {
                return Symbol::False();
            }
        }
        return Symbol::True();
    });

    names[">"] = h.Make<BuiltInProc<Number>>([](auto& args){
        for (size_t i = 0; i+1 < args.size(); ++i) {
            if (args[i]->GetValue() <= args[i + 1]->GetValue()) {
                return Symbol::False();
            }
        }
        return Symbol::True();
    });

    names["<="] = h.Make<BuiltInProc<Number>>([](auto& args){
        for (size_t i = 0; i+1 < args.size(); ++i) {
            if (args[i]->GetValue() > args[i + 1]->GetValue()) {
                return Symbol::False();
            }
        }
        return Symbol::True();
    });

    names[">="] = h.Make<BuiltInProc<Number>>([](auto& args){
        for (size_t i = 0; i+1 < args.size(); ++i) {
            if (args[i]->GetValue() < args[i + 1]->GetValue()) {
                return Symbol::False();
            }
        }
        return Symbol::True();
    });

    names["max"] = h.Make<BuiltInProc<Number>>([](auto& args){
        RequireSizeAtLeast<1>(args);
        int64_t value = args[0]->GetValue();
        for (size_t i = 1; i < args.size(); ++i) {
            value = std::max(value, args[i]->GetValue());
        }
        return Heap::Instance().Make<Number>(value);
    });

    names["min"] = h.Make<BuiltInProc<Number>>([](auto& args){
        RequireSizeAtLeast<1>(args);
        int64_t value = args[0]->GetValue();
        for (size_t i = 1; i < args.size(); ++i) {
            value = std::min(value, args[i]->GetValue());
        }
        return Heap::Instance().Make<Number>(value);
    });

    names["cons"] = h.Make<BuiltInProc<Object>>([](auto& args) {
        RequireSize<2>(args);
        return Heap::Instance().Make<Cell>(args[0], args[1]);
    });

    names["car"] = h.Make<BuiltInProc<Cell>>([](auto& args) {
        RequireSize<1>(args);
        return args[0]->GetFirst();
    });

    names["cdr"] = h.Make<BuiltInProc<Cell>>([](auto& args) {
        RequireSize<1>(args);
        return args[0]->GetSecond();
    });

    names["list"] = h.Make<BuiltInProc<Object>>([](auto& args) {
        Cell* list = nullptr;
        for (int i = args.size()-1; i >= 0; --i) {
            list = Heap::Instance().Make<Cell>(args[i], list);
        }
        return list;
    });

    names["list-ref"] = h.Make<BuiltInSyntax>([](auto ast, auto scope){
        auto args = ArgList(ast).ExpectSize(2);
        auto list = ArgList(args.EvalAs<Cell>(0, scope));
        auto index = args.EvalAs<Number>(1, scope);
        return list.At(index->GetValue());
    });

    names["list-tail"] = h.Make<BuiltInSyntax>([](auto ast, auto scope){
        auto args = ArgList(ast).ExpectSize(2);
        auto list = args.EvalAs<Cell>(0, scope);
        auto index = args.EvalAs<Number>(1, scope);
        int value = index->GetValue();
        while (--value) {
            list = As<Cell>(list->GetSecond());
        }
        return list->GetSecond();
    });

    names["if"] = h.Make<BuiltInSyntaxTailRecursive>([](auto ast, auto scope) -> Object* {
        auto args = ArgList(ast);
        if (args.Size() != 2 && args.Size() != 3) {
            throw SyntaxError("Wrong number of parameters");
        }
        auto cond = args.Eval(0, scope);
        if (EvalToTrue(cond)) {
            return args.At(1);
        }
        if (args.Size() == 3) {
            return args.At(2);
        }
        auto& h = Heap::Instance();
        return h.Make<Cell>(
            h.Make<Symbol>("quote"),
            h.Make<Cell>(nullptr, nullptr)
        );
    });

    names["begin"] = h.Make<BuiltInSyntaxTailRecursive>([](auto ast, auto scope){
        auto args = ArgList(ast).ExpectSizeAtLeast(1);
        for (size_t i = 0; i < args.Size()-1; ++i) {
            args.Eval(i, scope);
        }
        return args.At(args.Size()-1);
    });

    names["lambda"] = h.Make<BuiltInSyntax>([](auto ast, auto scope){
        auto args = ArgList(ast);
        if (args.Size() < 2) {
            throw SyntaxError("Invalid lambda expression.");
        }
        auto decl = ArgList(args.At(0));
        std::vector<Symbol*> formals;
        for (size_t i = 0; i < decl.Size(); ++i) {
            formals.push_back(As<Symbol>(decl.At(i)));
        }
        auto lambda_ast = Heap::Instance().Make<Cell>(
            Heap::Instance().Make<Symbol>("begin"),
            (As<Cell>(ast)->GetSecond())
        );
        return Heap::Instance().Make<Lambda>(formals, lambda_ast, scope);
    });

    names["define"] = h.Make<BuiltInSyntax>([](auto ast, auto scope){
        auto args = ArgList(ast);
        if (args.Size() < 2) {
            throw SyntaxError("Invalid define expression.");
        }
        auto declaration = args.At(0);
        if (Is<Symbol>(declaration)) {
            if (args.Size() != 2) {
                throw SyntaxError("Invalid define expression.");
            }
            scope->NewDefinition(As<Symbol>(declaration)->GetName(), args.Eval(1, scope));
        }
        else if (Is<Cell>(declaration)) {
            auto decl = ArgList(declaration);
            auto name = As<Symbol>(decl.At(0));
            std::vector<Symbol*> formals;
            for (size_t i = 1; i < decl.Size(); ++i) {
                formals.push_back(As<Symbol>(decl.At(i)));
            }
            auto lambda_ast = Heap::Instance().Make<Cell>(
                Heap::Instance().Make<Symbol>("begin"),
                (As<Cell>(ast)->GetSecond())
            );
            scope->NewDefinition(name->GetName(), Heap::Instance().Make<Lambda>(formals, lambda_ast, scope));
        } else {
            As<Symbol>(declaration);
        }
        return nullptr;
    });

    names["set!"] = h.Make<BuiltInSyntax>([](auto ast, auto scope){
        auto args = ArgList(ast);
        if (args.Size() != 2) {
            throw SyntaxError("Invalid set! expression.");
        }
        auto declaration = As<Symbol>(args.At(0));
        auto& name = As<Symbol>(declaration)->GetName();
        scope->SetDefinition(name, args.Eval(1, scope));
        return nullptr;
    });

    names["set-car!"] = h.Make<BuiltInProc<Object>>([](auto& args) {
        RequireSize<2>(args);
        As<Cell>(args[0])->SetFirst(args[1]);
        return nullptr;
    });

    names["set-cdr!"] = h.Make<BuiltInProc<Object>>([](auto& args) {
        RequireSize<2>(args);
        As<Cell>(args[0])->SetSecond(args[1]);
        return nullptr;
    });

    names["display"] = h.Make<BuiltInProc<Object>>([](auto& args) {
        RequireSize<1>(args);
        std::cout << ::ToString(args[0]) << std::endl;
        return nullptr;
    });


    return scope;
}

void Environment::NewDefinition(const std::string& name, Object* obj) {
    names_[name] = obj;
}
void Environment::SetDefinition(const std::string& name, Object* obj) {
    if (auto it = names_.find(name); it != names_.end()) {
        it->second = obj;
        return;
    }
    if (parent_) {
        parent_->SetDefinition(name, obj);
        return;
    }
    throw NameError("Trying to set! undefined variable.");
}
void Environment::SetParent(Environment* p) {
    parent_ = p;
}
Object* Environment::Eval(Environment*) {
    throw RuntimeError("Trying to evaluate Environment");
}
std::string Environment::ToString() const {
    std::string str = "Environment { ";
    for (auto [k, v] : names_) {
        (str += k) += " ";
    }
    return str + "}";
}
Object* Environment::GetDefinition(const std::string& name) {
    if (auto it = names_.find(name); it != names_.end()) {
        return it->second;
    }
    if (parent_) {
        return parent_->GetDefinition(name);
    }
    throw NameError("Invalid name: " + name);
}
void Environment::MarkDependencies() {
    for (auto& [k, v] : names_) {
        Heap::Instance().Mark(v);
    }
}