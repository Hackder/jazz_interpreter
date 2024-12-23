#include "token_pos.hpp"
#include "core.hpp"
#include "tokenizer.hpp"

void token_locator_init(TokenLocator* locator, String source, Arena* arena) {
    locator->source = source;
    array_init(&locator->line_offsets, 128, arena);

    for (isize i = 0; i < source.size; i++) {
        if (source[i] == '\n') {
            array_push(&locator->line_offsets, i + 1);
        }
    }
}

TokenPos token_locator_pos(TokenLocator* locator, String token) {
    core_assert(locator->source.data <= token.data);
    core_assert(locator->source.data + locator->source.size >=
                token.data + token.size);

    TokenPos pos = {};

    isize position = token.data - locator->source.data;

    // Binary search the correct line
    isize left = 0;
    isize right = locator->line_offsets.size;
    while (left < right) {
        isize mid = left + (right - left) / 2;
        if (locator->line_offsets[mid] <= position) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    pos.line = left;

    // Calculate the column
    if (pos.line == 0) {
        pos.column = position + 1;
    } else {
        pos.column = position - locator->line_offsets[pos.line - 1];
    }

    pos.line += 1;

    return pos;
}

TokenPos token_locator_pos(TokenLocator* locator, Token token) {
    return token_locator_pos(locator, token.source);
}

String token_locator_get_line(TokenLocator* locator, isize line) {
    core_assert(line > 0);
    core_assert(line <= locator->line_offsets.size + 1);

    isize start = 0;
    if (line > 1) {
        start = locator->line_offsets[line - 2];
    }

    isize end = locator->source.size;
    if (line <= locator->line_offsets.size) {
        end = locator->line_offsets[line - 1] - 1;
    }

    return string_substr(locator->source, start, end - start);
}
