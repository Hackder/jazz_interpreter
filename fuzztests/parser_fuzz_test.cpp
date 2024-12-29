#include "core.hpp"
#include "parser.hpp"
#include "tokenizer.hpp"
#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    String input = {.data = reinterpret_cast<const char*>(data),
                    .size = (isize)size};

    Arena arena;
    arena_init(&arena, 8 * 1024);
    defer(arena_free(&arena));

    Tokenizer tokenizer;
    tokenizer_init(&tokenizer, input);

    AstFile file;
    ast_file_init(&file, tokenizer, 64, &arena);

    ast_file_parse(&file, &arena);
    core_assert(ast_file_exhausted(&file));
    core_assert(arena_get_size(&arena) <= 64 * 1024 * 1024);

    return 0;
}
