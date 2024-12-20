#include "tokenizer.hpp"
#include <gtest/gtest.h>

const char* source_hello_world = R"(
main :: fn() {
  if 1 + 34 * 3 / 2 - 1 == 7 && 1 != 2 || 3 < 4 || 5 > 6 && 1 <= 2 && 3 >= 4 {
    dbg("Hi")
  }

  message := "Hello world"
  fmt.println(message)
}
)";

TEST(Tokenizer, ReadToken) {
    Tokenizer tokenizer;
    tokenzier_init(&tokenizer, string_from_cstr(source_hello_world));

    TokenizerResult result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Newline);
    EXPECT_EQ(result.token.source, string_from_cstr("\n"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Identifier);
    EXPECT_EQ(result.token.source, string_from_cstr("main"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Colon);
    EXPECT_EQ(result.token.source, string_from_cstr(":"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Colon);
    EXPECT_EQ(result.token.source, string_from_cstr(":"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Func);
    EXPECT_EQ(result.token.source, string_from_cstr("fn"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::LParen);
    EXPECT_EQ(result.token.source, string_from_cstr("("));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::RParen);
    EXPECT_EQ(result.token.source, string_from_cstr(")"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::LBrace);
    EXPECT_EQ(result.token.source, string_from_cstr("{"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Newline);
    EXPECT_EQ(result.token.source, string_from_cstr("\n"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::If);
    EXPECT_EQ(result.token.source, string_from_cstr("if"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Integer);
    EXPECT_EQ(result.token.source, string_from_cstr("1"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Plus);
    EXPECT_EQ(result.token.source, string_from_cstr("+"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Integer);
    EXPECT_EQ(result.token.source, string_from_cstr("34"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Asterisk);
    EXPECT_EQ(result.token.source, string_from_cstr("*"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Integer);
    EXPECT_EQ(result.token.source, string_from_cstr("3"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Slash);
    EXPECT_EQ(result.token.source, string_from_cstr("/"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Integer);
    EXPECT_EQ(result.token.source, string_from_cstr("2"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Minus);
    EXPECT_EQ(result.token.source, string_from_cstr("-"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Integer);
    EXPECT_EQ(result.token.source, string_from_cstr("1"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Equal);
    EXPECT_EQ(result.token.source, string_from_cstr("=="));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Integer);
    EXPECT_EQ(result.token.source, string_from_cstr("7"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::LogicalAnd);
    EXPECT_EQ(result.token.source, string_from_cstr("&&"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Integer);
    EXPECT_EQ(result.token.source, string_from_cstr("1"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::NotEqual);
    EXPECT_EQ(result.token.source, string_from_cstr("!="));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Integer);
    EXPECT_EQ(result.token.source, string_from_cstr("2"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::LogicalOr);
    EXPECT_EQ(result.token.source, string_from_cstr("||"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Integer);
    EXPECT_EQ(result.token.source, string_from_cstr("3"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::LessThan);
    EXPECT_EQ(result.token.source, string_from_cstr("<"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Integer);
    EXPECT_EQ(result.token.source, string_from_cstr("4"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::LogicalOr);
    EXPECT_EQ(result.token.source, string_from_cstr("||"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Integer);
    EXPECT_EQ(result.token.source, string_from_cstr("5"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::GreaterThan);
    EXPECT_EQ(result.token.source, string_from_cstr(">"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Integer);
    EXPECT_EQ(result.token.source, string_from_cstr("6"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::LogicalAnd);
    EXPECT_EQ(result.token.source, string_from_cstr("&&"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Integer);
    EXPECT_EQ(result.token.source, string_from_cstr("1"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::LessEqual);
    EXPECT_EQ(result.token.source, string_from_cstr("<="));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Integer);
    EXPECT_EQ(result.token.source, string_from_cstr("2"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::LogicalAnd);
    EXPECT_EQ(result.token.source, string_from_cstr("&&"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Integer);
    EXPECT_EQ(result.token.source, string_from_cstr("3"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::GreaterEqual);
    EXPECT_EQ(result.token.source, string_from_cstr(">="));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Integer);
    EXPECT_EQ(result.token.source, string_from_cstr("4"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::LBrace);
    EXPECT_EQ(result.token.source, string_from_cstr("{"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Newline);
    EXPECT_EQ(result.token.source, string_from_cstr("\n"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Identifier);
    EXPECT_EQ(result.token.source, string_from_cstr("dbg"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::LParen);
    EXPECT_EQ(result.token.source, string_from_cstr("("));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::String);
    EXPECT_EQ(result.token.source, string_from_cstr("Hi"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::RParen);
    EXPECT_EQ(result.token.source, string_from_cstr(")"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Newline);
    EXPECT_EQ(result.token.source, string_from_cstr("\n"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::RBrace);
    EXPECT_EQ(result.token.source, string_from_cstr("}"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Newline);
    EXPECT_EQ(result.token.source, string_from_cstr("\n"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Identifier);
    EXPECT_EQ(result.token.source, string_from_cstr("message"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Colon);
    EXPECT_EQ(result.token.source, string_from_cstr(":"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Assign);
    EXPECT_EQ(result.token.source, string_from_cstr("="));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::String);
    EXPECT_EQ(result.token.source, string_from_cstr("Hello world"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Newline);
    EXPECT_EQ(result.token.source, string_from_cstr("\n"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Identifier);
    EXPECT_EQ(result.token.source, string_from_cstr("fmt"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Period);
    EXPECT_EQ(result.token.source, string_from_cstr("."));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Identifier);
    EXPECT_EQ(result.token.source, string_from_cstr("println"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::LParen);
    EXPECT_EQ(result.token.source, string_from_cstr("("));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Identifier);
    EXPECT_EQ(result.token.source, string_from_cstr("message"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::RParen);
    EXPECT_EQ(result.token.source, string_from_cstr(")"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Newline);
    EXPECT_EQ(result.token.source, string_from_cstr("\n"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::RBrace);
    EXPECT_EQ(result.token.source, string_from_cstr("}"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Newline);
    EXPECT_EQ(result.token.source, string_from_cstr("\n"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Eof);
    EXPECT_EQ(result.token.source, string_from_cstr(""));
}

TEST(Tokenizer, Errors) {
    Tokenizer tokenizer;
    tokenzier_init(&tokenizer, string_from_cstr("asdf~"));

    TokenizerResult result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::None);
    EXPECT_EQ(result.token.kind, TokenKind::Identifier);
    EXPECT_EQ(result.token.source, string_from_cstr("asdf"));

    result = tokenizer_next_token(&tokenizer);
    EXPECT_EQ(result.error, TokenizerErrorKind::InvalidCharacter);
    EXPECT_EQ(result.token.kind, TokenKind::Invalid);
    EXPECT_EQ(result.token.source, string_from_cstr("~"));
}
