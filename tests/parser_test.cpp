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

    AstNode* node = parse_expression(file, false, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Bin(Bin(Lit(1) + Lit(2)) + Lit(3))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, ExprNumbersAndParens) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("1 + (2 + 3)", &arena);

    AstNode* node = parse_expression(file, false, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Bin(Lit(1) + Bin(Lit(2) + Lit(3)))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, ExprUnary) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("-1 + +2 - -3", &arena);

    AstNode* node = parse_expression(file, false, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(
        ast.data,
        "Bin(Bin(Unary(- Lit(1)) + Unary(+ Lit(2))) - Unary(- Lit(3)))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, ExprCorrectPrecedence) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("1 + 2 * 3 - 3 - 1 / 3 + 2", &arena);

    AstNode* node = parse_expression(file, false, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Bin(Bin(Bin(Bin(Lit(1) + Bin(Lit(2) * Lit(3))) - "
                           "Lit(3)) - Bin(Lit(1) / Lit(3))) + Lit(2))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, ExprIdent) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("1 + asdf * (thing - b)", &arena);

    AstNode* node = parse_expression(file, false, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Bin(Lit(1) + Bin(Ident(asdf) * "
                           "Bin(Ident(thing) - Ident(b))))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, DeclSimpleVariable) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("thing := 1", &arena);

    AstNode* node = parse_declaration(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Decl(thing := Lit(1))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, DeclSimpleConstant) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("thing :: 1", &arena);

    AstNode* node = parse_declaration(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Decl(thing :: Lit(1))");
    EXPECT_TRUE(ast_file_exhausted(file));
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
    EXPECT_TRUE(ast_file_exhausted(file));
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
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, Block) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        {
            1 + 1
        }
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_statement(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Block(Bin(Lit(1) + Lit(1)))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, Break) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        {
            break 1 + 1
        }
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_statement(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Block(Break(Bin(Lit(1) + Lit(1))))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, ForExpr) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = "for i := 0; i < 10; i = i + 1 { 1 + 2 }";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_statement(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "For(Decl(i := Lit(0)) Bin(Ident(i) < Lit(10)) "
                           "Assign(Ident(i) Bin(Ident(i) + Lit(1))) then "
                           "Block(Bin(Lit(1) + Lit(2))))");
    EXPECT_TRUE(ast_file_exhausted(file));
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
                 "Assign(Ident(i) Bin(Ident(i) + Lit(1))) then "
                 "Block(Break(Bin(Lit(1) + Lit(2)))) else Block(Lit(3)))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, ForExprWhile) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        for i < 10 {
            break 1 + 2
        } else {
            3
        }
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_statement(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data,
                 "For(Bin(Ident(i) < Lit(10)) then Block(Break(Bin(Lit(1) "
                 "+ Lit(2)))) else Block(Lit(3)))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, ForExprInfinite) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        for {
            break 1 + 2
        } else {
            3
        }
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_statement(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "For(then Block(Break(Bin(Lit(1) "
                           "+ Lit(2)))) else Block(Lit(3)))");
    EXPECT_TRUE(ast_file_exhausted(file));
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
    EXPECT_TRUE(ast_file_exhausted(file));
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
    EXPECT_TRUE(ast_file_exhausted(file));
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
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, StructFieldAccess) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        another.something + 1
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_expression(file, true, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data,
                 "Bin(Bin(Ident(another) . Ident(something)) + Lit(1))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, StructFieldAssignable) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        another.something = 1
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_statement(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data,
                 "Assign(Bin(Ident(another) . Ident(something)) Lit(1))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, ArrayIndex) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        another.something[i] + 1
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_expression(file, true, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(
        ast.data,
        "Bin(Bin(Bin(Ident(another) . Ident(something)) [ Ident(i)) + Lit(1))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, ArrayIndexAssignable) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        another[i] = 1
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_statement(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Assign(Bin(Ident(another) [ Ident(i)) Lit(1))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, ArrayIndexFunctionCallStructFieldAccess) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        another.something[i](123)(34)[12](33).window(a, b, 34)[12]
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_expression(file, true, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(
        ast.data,
        "Bin(Call(Bin(Call(Bin(Call(Call(Bin(Bin(Ident(another) . "
        "Ident(something)) [ Ident(i)) Lit(123)) Lit(34)) [ Lit(12)) Lit(33)) "
        ". Ident(window)) Ident(a) Ident(b) Lit(34)) [ Lit(12))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, MultipleStatementsWithinBlock) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        if true {
            a := this + 1
            b := a + 2

            c := house[i].tree()
            b = c.window(a, b, 34)[12]
        }
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_statement(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(
        ast.data,
        "If(Lit(true) then Block(Decl(a := Bin(Ident(this) + Lit(1))) Decl(b "
        ":= Bin(Ident(a) + Lit(2))) Decl(c := Call(Bin(Bin(Ident(house) [ "
        "Ident(i)) . Ident(tree)))) Assign(Ident(b) Bin(Call(Bin(Ident(c) . "
        "Ident(window)) Ident(a) Ident(b) Lit(34)) [ Lit(12)))))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, SimpleErrors) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        if true {
            a := 1 +
            b := a + 2
        }
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_statement(file, &arena);
    for (int i = 0; i < file->errors.size; i++) {
        ParseError error = file->errors[i];
        std::cerr << error.token.kind << ": " << error.message << std::endl;
    }
    EXPECT_TRUE(ast_file_exhausted(file));
}
