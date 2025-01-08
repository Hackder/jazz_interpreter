#include "common.hpp"
#include "core.hpp"
#include "parser.hpp"
#include "sema.hpp"
#include <gtest/gtest.h>

TEST(Sema, BasicDecl) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        main :: fn() {
            1
        }
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    ast_file_parse(file, &arena);
    EXPECT_EQ(file->errors.size, 0);

    semantic_analysis(file, &arena);
}

TEST(Sema, MultipleBasicDecl) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        main :: fn() {
            1
        }

        another :: fn() {
            2
        }

        yet_another :: fn() {
            3
        }
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    ast_file_parse(file, &arena);
    EXPECT_EQ(file->errors.size, 0);

    semantic_analysis(file, &arena);
}

TEST(Sema, SimpleTypecheck) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        main :: fn() {
            a := 1
            b := 3

            c := a + b

            d := 8
            e := c + d
        }
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    ast_file_parse(file, &arena);
    EXPECT_EQ(file->errors.size, 0);

    semantic_analysis(file, &arena);
}

TEST(Sema, SimpleTypecheckFunctions) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        identity :: fn(a) {
            return a + identity(1)
        }
        
        sum :: fn(a, b) {
            return a + identity(b)
        }

        main :: fn() {
            c := sum(1, 3)
        }
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    ast_file_parse(file, &arena);
    EXPECT_EQ(file->errors.size, 0);

    if (file->errors.size > 0) {
        TokenLocator locator;
        token_locator_init(&locator, string_from_cstr(source), &arena);

        for (isize i = 0; i < file->errors.size; i++) {
            ParseError error = file->errors[i];
            // std::cerr << error.token.kind << ": " << error.token.source
            //           << std::endl;
            Array<String> parts =
                parse_error_pretty_print(&error, &locator, &arena);

            for (isize j = 0; j < parts.size; j++) {
                std::cout << parts[j];
            }
            std::cout << std::endl;
        }
    }

    semantic_analysis(file, &arena);
}

// TEST(Sema, Complex) {
//     Arena arena;
//     arena_init(&arena, 2048);
//     defer(arena_free(&arena));
//     const char* source = R"SOURCE(
//         sum :: fn(a: int, b: int) -> int {
//             return a + b
//         }
//
//         main :: fn() {
//             sum(1, 2)
//
//             jack := 1
//             jill := 2
//             if jack == jill {
//                 return sum(jack, jill)
//             }
//
//             sum(jack, jill)
//
//             tom := if jack == 1 { 10 } else { 20 }
//
//             for i := 0; i < 10; i = i + 1 {
//                 if i == jill {
//                     break
//                 }
//             }
//
//             joe := for {
//                 break 123
//             }
//
//             {
//                 bill := 1
//                 bill = bill + 1
//             }
//
//             joe = 5
//
//             return joe
//         }
//     )SOURCE";
//     AstFile* file = setup_ast_file(source, &arena);
//
//     ast_file_parse(file, &arena);
//     EXPECT_EQ(file->errors.size, 0);
//
//     semantic_analysis(file, &arena);
// }
