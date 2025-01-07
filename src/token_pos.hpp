#pragma once

#include "core.hpp"
#include "tokenizer.hpp"

struct TokenLocator {
    String source;
    Array<isize> line_offsets;
};

void token_locator_init(TokenLocator* locator, String source, Arena* arena);

struct TokenPos {
    // 1-based index
    isize line;
    // 1-based index
    isize column;
};

TokenPos token_locator_pos(TokenLocator* locator, String token);
TokenPos token_locator_pos(TokenLocator* locator, Token token);
String token_locator_get_line(TokenLocator* locator, isize line);
