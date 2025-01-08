#include "common.hpp"
#include "core.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>

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

TEST(Parser, DeclVariableWithExplicitType) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("a:int = 1", &arena);

    AstNode* node = parse_declaration(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Decl(a :Ident(int) = Lit(1))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, DeclConstantWithExplicitType) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    AstFile* file = setup_ast_file("a:int : 1", &arena);

    AstNode* node = parse_declaration(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data, "Decl(a :Ident(int) : Lit(1))");
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

    parse_statement(file, &arena);
    ParseError error = file->errors[0];
    EXPECT_EQ(error.token.kind, TokenKind::Newline);
    EXPECT_STREQ(error.message.data, "Unexpected newline");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, AssignmentInvalid) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        {
            a + 1 := somethign
        }
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    parse_statement(file, &arena);
    ParseError error = file->errors[0];
    EXPECT_EQ(error.token.kind, TokenKind::Colon);
    EXPECT_STREQ(error.message.data, "Invalid declaration");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, ReturnWithValue) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        sum :: fn(a: int, b: int) -> int {
            return a + b
        }
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    AstNode* node = parse_statement(file, &arena);
    String ast = ast_serialize_debug(node, &arena);
    EXPECT_STREQ(ast.data,
                 "Decl(sum :: Func(fn -> Ident(int), Param(a) Param(b) "
                 "Block(Return(Bin(Ident(a) + Ident(b))))))");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, UnclosedParentheses) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        t::r(
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    parse_statement(file, &arena);
    ParseError error = file->errors[0];
    EXPECT_EQ(error.token.kind, TokenKind::Eof);
    EXPECT_STREQ(error.message.data, "Unexpected token");
    EXPECT_TRUE(ast_file_exhausted(file));
}

TEST(Parser, InfiniteLoop) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const unsigned char source_data[] = {
        0x6e, 0x6e, 0x6e, 0x3a, 0x3a, 0x2a, 0x28, 0x2a, 0x28, 0x28, 0x28, 0x28,
        0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0xd8,
        0xdd, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28,
        0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28,
        0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28,
        0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28,
        0x28, 0x28, 0x28, 0x28, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b,
        0x2b, 0x2b, 0x2b, 0x29, 0x2a, 0x28, 0x28, 0x42, 0x2a, 0xff, 0x35, 0x2b,
        0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b,
        0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b,
        0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b,
        0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b,
        0xdf, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x28, 0x28,
        0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28,
        0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28,
        0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28,
        0x28, 0x28, 0x28, 0x28, 0x28, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b,
        0x2b, 0x2b, 0x2b, 0x2b, 0x29, 0x2a, 0x28, 0x28, 0x42, 0x2a, 0xff, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x35, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b,
        0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b,
        0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b,
        0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b,
        0x2b, 0x2b, 0x2b, 0x2b, 0xdf, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x2b,
        0x2b, 0x2b, 0x2b, 0x2b, 0x2b, 0x28, 0x29};

    String source = {.data = (const char*)source_data,
                     .size = sizeof(source_data)};

    Tokenizer tokenizer;
    tokenizer_init(&tokenizer, source);
    AstFile* file = ast_file_make(tokenizer, 16, &arena);
    ast_file_parse(file, &arena);

    core_assert(ast_file_exhausted(file));
    core_assert(arena_get_size(&arena) <= 128 * 1024);
}

TEST(Parser, HighMemoryUsage) {
    Arena arena;
    arena_init(&arena, 128 * 1024);
    defer(arena_free(&arena));
    const unsigned char source_data[] = {
        0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x4e, 0x5e, 0x5e, 0x5e, 0x5e,
        0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e,
        0xee, 0xee, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xee, 0xee, 0xee, 0xd5,
        0x26, 0x0f, 0x6f, 0x72, 0x3c, 0xee, 0x41, 0x3a, 0x3d, 0x66, 0x6f, 0x72,
        0x3c, 0xee, 0x0f, 0x41, 0x3a, 0x3d, 0x66, 0x6f, 0x72, 0x20, 0x00, 0x00,
        0x00, 0x3a, 0x41, 0x3a, 0x3d, 0xb4, 0xee, 0xee, 0xee, 0x24, 0xee, 0xee,
        0xee, 0xee, 0xee, 0xee, 0xee, 0x3e, 0x2b, 0x3e, 0x3e, 0xff, 0xee, 0xee,
        0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0x12, 0x03, 0x00, 0x06, 0x00,
        0x00, 0xee, 0xee, 0xee, 0x41, 0x3a, 0x3d, 0x66, 0x0a, 0xd2, 0x3c, 0xee,
        0x0f, 0x41, 0x3a, 0x3d, 0x66, 0x6f, 0x72, 0x20, 0x00, 0x00, 0x00, 0x3a,
        0x41, 0x3a, 0x3d, 0xb4, 0xee, 0xee, 0xee, 0x24, 0xee, 0xee, 0xee, 0xee,
        0xee, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b,
        0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b,
        0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0xab, 0x8b,
        0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x3c, 0x3c, 0x74,
        0x72, 0x75, 0x3c, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b,
        0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b, 0x8b,
        0x8b, 0x8b, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e,
        0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e,
        0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x8e, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e,
        0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x3a, 0x5e, 0x5e, 0x5e,
        0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e,
        0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e,
        0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e,
        0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e,
        0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x83

    };

    String source = {.data = (const char*)source_data,
                     .size = sizeof(source_data)};

    Tokenizer tokenizer;
    tokenizer_init(&tokenizer, source);
    AstFile* file = ast_file_make(tokenizer, 128, &arena);
    ast_file_parse(file, &arena);

    core_assert(ast_file_exhausted(file));
    core_assert(arena_get_size(&arena) <= 128 * 1024);
}

TEST(Parser, TooManyErrors) {
    Arena arena;
    arena_init(&arena, 128 * 1024);
    defer(arena_free(&arena));

    const unsigned char source_data[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x02, 0x00, 0x00,
        0x66, 0xe4, 0xff, 0xff, 0xff, 0x5f, 0xaf};

    String source = {.data = (const char*)source_data,
                     .size = sizeof(source_data)};

    Tokenizer tokenizer;
    tokenizer_init(&tokenizer, source);
    AstFile* file = ast_file_make(tokenizer, 128, &arena);
    ast_file_parse(file, &arena);

    core_assert(ast_file_exhausted(file));
    core_assert(arena_get_size(&arena) <= 128 * 1024);
    core_assert(file->errors.size <= 100);
}
