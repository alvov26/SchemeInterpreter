#include <scheme/tokenizer.h>

#include <cctype>

bool SymbolToken::operator==(const SymbolToken &other) const {
    return name == other.name;
}

bool QuoteToken::operator==(const QuoteToken &) const {
    return true;
}

bool DotToken::operator==(const DotToken &) const {
    return true;
}

bool ConstantToken::operator==(const ConstantToken &other) const {
    return value == other.value;
}

Tokenizer::Tokenizer(std::istream *in) : in_(in) {
    Next();
}

bool Tokenizer::IsEnd() {
    return !current_token_.has_value();
}

void Tokenizer::Next() {
    int next_char;
    do {
        if (in_->eof()) {
            current_token_.reset();
            return;
        }
        next_char = in_->get();
    } while (std::isblank(next_char) || next_char == '\n' || next_char == '\r');

    if (next_char == '(') {
        current_token_ = Token{BracketToken::OPEN};
    } else if (next_char == ')') {
        current_token_ = Token{BracketToken::CLOSE};
    } else if (next_char == '.') {
        current_token_ = Token{DotToken{}};
    } else if (next_char == '\'') {
        current_token_ = Token{QuoteToken{}};
    } else if (std::isdigit(next_char)) {
        in_->unget();
        current_token_ = Token{ReadConstant()};
    } else if (next_char == '+') {
        next_char = in_->get();
        in_->unget();
        if (std::isdigit(next_char)) {
            current_token_ = Token{ReadConstant()};
        } else {
            current_token_ = Token{SymbolToken{"+"}};
        }
    } else if (next_char == '-') {
        next_char = in_->get();
        in_->unget();
        if (std::isdigit(next_char)) {
            in_->unget(); // return minus to stream
            current_token_ = Token{ReadConstant()};
        } else {
            current_token_ = Token{SymbolToken{"-"}};
        }
    } else if (std::isalpha(next_char)
               || next_char == '<' || next_char == '=' || next_char == '>'
               || next_char == '*' || next_char == '/' || next_char == '#') {
        in_->unget();
        current_token_ = Token{ReadSymbol()};
    } else {
        current_token_.reset();
    }
}

Token Tokenizer::GetToken() {
    return *current_token_;
}

ConstantToken Tokenizer::ReadConstant() {
    int value;
    *in_ >> value;
    return ConstantToken{value};
}

SymbolToken Tokenizer::ReadSymbol() {
    std::string name;
    int c = in_->get();
    while (std::isalnum(c)
           || c == '<' || c == '=' || c == '>'
           || c == '*' || c == '/' || c == '#'
           || c == '?' || c == '!' || c == '-') {
        name.push_back(c);
        c = in_->get();
    }
    in_->unget();
    return SymbolToken{name};
}
