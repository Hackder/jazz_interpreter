#include "core.hpp"
#include "token_pos.hpp"
#include <gtest/gtest.h>

TEST(TokenPos, EmptySource) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));

    String source = string_from_cstr("");

    TokenLocator locator;
    token_locator_init(&locator, source, &arena);

    TokenPos pos = token_locator_pos(&locator, source);
    EXPECT_EQ(pos.line, 1);
    EXPECT_EQ(pos.column, 1);
}

TEST(TokenPos, TokenPos) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));

    String source = string_from_cstr("hello\nworld\nsomething");

    TokenLocator locator;
    token_locator_init(&locator, source, &arena);

    {
        String token = string_substr(source, 8, 3);
        EXPECT_EQ(token, string_from_cstr("rld"));
        TokenPos pos = token_locator_pos(&locator, token);
        EXPECT_EQ(pos.line, 2);
        EXPECT_EQ(pos.column, 3);
    }

    {
        String token = string_substr(source, 0, 1);
        EXPECT_EQ(token, string_from_cstr("h"));
        TokenPos pos = token_locator_pos(&locator, token);
        EXPECT_EQ(pos.line, 1);
        EXPECT_EQ(pos.column, 1);
    }

    {
        String token = string_substr(source, 11, 4);
        EXPECT_EQ(token, string_from_cstr("\nsom"));
        TokenPos pos = token_locator_pos(&locator, token);
        EXPECT_EQ(pos.line, 2);
        EXPECT_EQ(pos.column, 6);
    }

    {
        String token = string_substr(source, 18, 3);
        EXPECT_EQ(token, string_from_cstr("ing"));
        TokenPos pos = token_locator_pos(&locator, token);
        EXPECT_EQ(pos.line, 3);
        EXPECT_EQ(pos.column, 7);
    }
}

TEST(TokenPos, GetLine) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));

    String source = string_from_cstr("hello\nworld\nsomething");

    TokenLocator locator;
    token_locator_init(&locator, source, &arena);

    {
        String line = token_locator_get_line(&locator, 1);
        EXPECT_EQ(line, string_from_cstr("hello"));
    }
    {
        String line = token_locator_get_line(&locator, 2);
        EXPECT_EQ(line, string_from_cstr("world"));
    }
    {
        String line = token_locator_get_line(&locator, 3);
        EXPECT_EQ(line, string_from_cstr("something"));
    }
}
