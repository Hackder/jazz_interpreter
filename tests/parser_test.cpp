#include "core.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>

AstFile* setup_ast_file(const char* source, Arena* arena) {
    Tokenizer tokenizer;
    tokenizer_init(&tokenizer, string_from_cstr(source));

    AstFile* file = ast_file_make(tokenizer, 16, arena);

    return file;
}

TEST(Parser, ExprNumbersOnly) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("1 + 2 + 3", &arena);

    AstNode* node = parse_expression(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Bin(Bin(Lit(1) + Lit(2)) + Lit(3))");
}

TEST(Parser, ExprNumbersAndParens) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("1 + (2 + 3)", &arena);

    AstNode* node = parse_expression(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Bin(Lit(1) + Bin(Lit(2) + Lit(3)))");
}

TEST(Parser, ExprUnary) {
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

TEST(Parser, ExprCorrectPrecedence) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("1 + 2 * 3 - 3 - 1 / 3 + 2", &arena);

    AstNode* node = parse_expression(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Bin(Bin(Bin(Bin(Lit(1) + Bin(Lit(2) * Lit(3))) - "
                           "Lit(3)) - Bin(Lit(1) / Lit(3))) + Lit(2))");
}

TEST(Parser, ExprIdent) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("1 + asdf * (thing - b)", &arena);

    AstNode* node = parse_expression(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Bin(Lit(1) + Bin(Ident(asdf) * "
                           "Bin(Ident(thing) - Ident(b))))");
}

TEST(Parser, DeclSimpleVariable) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("thing := 1", &arena);

    AstNode* node = parse_declaration(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Decl(thing := Lit(1))");
}

TEST(Parser, DeclSimpleConstant) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("thing :: 1", &arena);

    AstNode* node = parse_declaration(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Decl(thing :: Lit(1))");
}

TEST(Parser, DeclSimpleFunction) {
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

TEST(Parser, DeclSimpleFunctionReturnType) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source =
        "main :: fn(para: int, another) -> int { para + another }";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_declaration(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(
        ast.data,
        "Decl(main :: Func(fn -> Ident(int), Param(para) Param(another) "
        "Block(Bin(Ident(para) + Ident(another)))))");
}

TEST(Parser, ForExpr) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = "for i := 0; i < 10; i = i + 1 { 1 + 2 }";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_statement(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(
        ast.data,
        "For(Decl(i := Lit(0)) Bin(Ident(i) < Lit(10)) "
        "Assign(i Bin(Ident(i) + Lit(1))) then Block(Bin(Lit(1) + Lit(2))))");
}

TEST(Parser, ForExprWithElse) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        for i := 0; i < 10; i = i + 1 {
            break 1 + 2
        } else {
            3
        }
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_statement(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data,
                 "For(Decl(i := Lit(0)) Bin(Ident(i) < Lit(10)) "
                 "Assign(i Bin(Ident(i) + Lit(1))) then "
                 "Block(Break(Bin(Lit(1) + Lit(2)))) else Block(Lit(3)))");
}

TEST(Parser, IfExpr) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        if i + 2 == 3 {
            1 + 2
        }
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_statement(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(
        ast.data,
        "If(Bin(Bin(Ident(i) + Lit(2)) == Lit(3)) then Block(Bin(Lit(1) + "
        "Lit(2))))");
}

TEST(Parser, IfExprWithElse) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        if i + 2 == 3 {
            1 + 2
        } else {
            hello
        }
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_statement(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(
        ast.data,
        "If(Bin(Bin(Ident(i) + Lit(2)) == Lit(3)) then Block(Bin(Lit(1) + "
        "Lit(2))) else Block(Ident(hello)))");
}

TEST(Parser, IfExprWithElseAssigned) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        value := if i + 2 == 3 {
            1 + 2
        } else {
            hello
        }
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_statement(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Decl(value := If(Bin(Bin(Ident(i) + Lit(2)) == "
                           "Lit(3)) then Block(Bin(Lit(1) + "
                           "Lit(2))) else Block(Ident(hello))))");
}
