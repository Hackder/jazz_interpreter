#pragma once

#include "parser.hpp"

inline AstFile* setup_ast_file(const char* source, Arena* arena) {
    Tokenizer tokenizer;
    tokenizer_init(&tokenizer, string_from_cstr(source));

    AstFile* file = ast_file_make(tokenizer, 16, arena);

    return file;
}
