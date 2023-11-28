#include <scheme/parser.h>
#include <scheme/error.h>
#include <scheme/heap.h>

Object* ReadList(Tokenizer *tokenizer) {
    Heap& h = Heap::Instance();

    if (tokenizer->IsEnd()) {
        throw SyntaxError("Tokenizer is end");
    }
    Token next_token = tokenizer->GetToken();
    if (auto* ptr = std::get_if<BracketToken>(&next_token)) {
        if (*ptr == BracketToken::CLOSE) {
            tokenizer->Next();
            return nullptr;
        }
    }
    Object* first = Read(tokenizer);
    if (tokenizer->IsEnd()) {
        throw SyntaxError("Tokenizer is end");
    }
    next_token = tokenizer->GetToken();
    if (auto* ptr = std::get_if<DotToken>(&next_token)) {
        tokenizer->Next();
        Object* second = Read(tokenizer);
        if (tokenizer->IsEnd()) {
            throw SyntaxError("Tokenizer is end");
        }
        next_token = tokenizer->GetToken();
        if (auto* ptr2 = std::get_if<BracketToken>(&next_token)) {
            if (*ptr2 != BracketToken::CLOSE) {
                throw SyntaxError("Missing )");
            }
        } else {
            throw SyntaxError("Missing )");
        }
        tokenizer->Next();
        return h.Make<Cell>(first, second);
    }
    return h.Make<Cell>(first, ReadList(tokenizer));
}

Object* Read(Tokenizer *tokenizer) {
    Heap& h = Heap::Instance();

    if (tokenizer->IsEnd()) {
        throw SyntaxError("Tokenizer is end");
    }

    Token next_token = tokenizer->GetToken();
    tokenizer->Next();
    if (auto* ptr = std::get_if<ConstantToken>(&next_token)) {
        return h.Make<Number>(ptr->value);
    }
    if (auto* ptr = std::get_if<SymbolToken>(&next_token)) {
        return h.Make<Symbol>(std::move(ptr->name));
    }
    if (auto* ptr = std::get_if<BracketToken>(&next_token)) {
        switch (*ptr) {
            case BracketToken::OPEN: return ReadList(tokenizer);
            case BracketToken::CLOSE: throw SyntaxError("Unexpected ')' detected");
        }
    }
    if (auto* ptr = std::get_if<QuoteToken>(&next_token)) {
        return h.Make<Cell>(h.Make<Symbol>("quote"), h.Make<Cell>(Read(tokenizer), nullptr));
    }
    if (auto* ptr = std::get_if<DotToken>(&next_token)) {
        throw SyntaxError("Unexpected '.' detected");
    }
    throw SyntaxError("Unknown token detected");
}