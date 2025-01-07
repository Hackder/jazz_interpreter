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

TEST(Sema, Complex) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));
    const char* source = R"SOURCE(
        sum :: fn(a: int, b: int) -> int {
            return a + b
        }

        main :: fn() {
            sum(1, 2)
            
            jack := 1
            jill := 2
            if jack == jill {
                return sum(jack, jill)
            }

            sum(jack, jill)

            tom := if jack == 1 { 10 } else { 20 }

            for i := 0; i < 10; i = i + 1 {
                if i == jill {
                    break
                }
            }

            joe := for {
                break 123
            }

            {
                bill := 1
                bill = bill + 1
            }

            joe = 5

            return joe
        }
    )SOURCE";
    AstFile* file = setup_ast_file(source, &arena);

    ast_file_parse(file, &arena);
    EXPECT_EQ(file->errors.size, 0);

    semantic_analysis(file, &arena);
}
