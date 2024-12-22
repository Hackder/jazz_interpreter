#include "core.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>

AstFile* setup_ast_file(const char* source, Arena* arena) {
    Tokenizer tokenizer;
    tokenzier_init(&tokenizer, string_from_cstr(source));

    AstFile* file = ast_file_make(tokenizer, 16, arena);

    return file;
}

TEST(Parser, ParseExprNumbersOnly) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("1 + 2 + 3", &arena);

    AstNode* node = parse_expression(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Bin(Bin(Lit(1) + Lit(2)) + Lit(3))");
}

TEST(Parser, ParseExprNumbersAndParens) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("1 + (2 + 3)", &arena);

    AstNode* node = parse_expression(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Bin(Lit(1) + Bin(Lit(2) + Lit(3)))");
}

TEST(Parser, ParseExprUnary) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("-1 + +2 - -3", &arena);

    AstNode* node = parse_expression(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(
        ast.data,
        "Bin(Bin(Unary(- Lit(1)) + Unary(+ Lit(2))) - Unary(- Lit(3)))");
}

TEST(Parser, ParseExprCorrectPrecedence) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("1 + 2 * 3 - 3 - 1 / 3 + 2", &arena);

    AstNode* node = parse_expression(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Bin(Bin(Bin(Bin(Lit(1) + Bin(Lit(2) * Lit(3))) - "
                           "Lit(3)) - Bin(Lit(1) / Lit(3))) + Lit(2))");
}

TEST(Parser, ParseExprIdent) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("1 + asdf * (thing - b)", &arena);

    AstNode* node = parse_expression(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Bin(Lit(1) + Bin(Ident(asdf) * "
                           "Bin(Ident(thing) - Ident(b))))");
}

TEST(Parser, ParseDeclSimpleVariable) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("thing := 1", &arena);

    AstNode* node = parse_declaration(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Decl(thing := Lit(1))");
}

TEST(Parser, ParseDeclSimpleConstant) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("thing :: 1", &arena);

    AstNode* node = parse_declaration(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Decl(thing :: Lit(1))");
}

TEST(Parser, ParserDeclSimpleFunction) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = "main :: fn(para: int, another) { 1 + 2 }";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_declaration(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Decl(main :: Func(fn Param(para) Param(another) "
                           "Block(Bin(Lit(1) + Lit(2)))))");
}
