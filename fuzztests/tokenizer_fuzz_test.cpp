#include "tokenizer.hpp"
#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    String input = {.data = reinterpret_cast<const char*>(data),
                    .size = (isize)size};

    Tokenizer tokenizer;
    tokenizer_init(&tokenizer, input);

    while (true) {
        TokenizerResult result = tokenizer_next_token(&tokenizer);
        // The return token is from the source string
        core_assert(result.token.source.data >= input.data);
        core_assert(result.token.source.data + result.token.source.size <=
                    input.data + input.size);

        if (result.error != TokenizerErrorKind::None) {
            continue;
        }

        if (result.token.kind == TokenKind::Eof) {
            break;
        }
    }

    return 0;
}
