#include <catch2/catch_all.hpp>

#include <fuzzer.h>

#include <scheme/tokenizer.h>
#include <scheme/parser.h>
#include <scheme/error.h>

#include <iostream>

static constexpr uint32_t kShotsCount = 100000;

TEST_CASE("Fuzzing-1") {
    Fuzzer fuzzer;

    for (uint32_t i = 0; i < kShotsCount; ++i) {
        try {
            auto req = fuzzer.Next();
#ifdef SCHEME_FUZZING_1_PRINT_REQUESTS
            std::cerr << "[ " << i << " ] " << req << std::endl;
#endif
            std::stringstream ss{req};
            Tokenizer tokenizer{&ss};
            while (!tokenizer.IsEnd()) {
                Read(&tokenizer);
            }
        } catch (const SyntaxError&) {
        }
    }
}
