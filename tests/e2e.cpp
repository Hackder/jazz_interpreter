#include "compiler.hpp"
#include "core.hpp"
#include "parser.hpp"
#include "sema.hpp"
#include "vm.hpp"
#include <cstdio>
#include <gtest/gtest.h>
#include <iostream>

u8 execute_to_end(const char* source_code_str, FILE* stdout_file,
                  FILE* stderr_file) {
    Arena arena;
    arena_init(&arena, 16 * 1024);
    defer(arena_free(&arena));
    // defer({
    //     std::cerr << "Interpreter memory used: " << arena_get_size(&arena)
    //               << " bytes" << std::endl;
    // });

    String source_code = string_from_cstr(source_code_str);

    Tokenizer tokenizer;
    tokenizer_init(&tokenizer, source_code);
    AstFile* file = ast_file_make(tokenizer, 16, &arena);
    ast_file_parse(file, &arena);

    core_assert(file->errors.size == 0);

    // if (file->errors.size > 0) {
    //     TokenLocator locator;
    //     token_locator_init(&locator, source_code, &arena);
    //
    //     for (isize i = 0; i < file->errors.size; i++) {
    //         ParseError error = file->errors[i];
    //         // std::cerr << error.token.kind << ": " << error.token.source
    //         //           << std::endl;
    //         Array<String> parts =
    //             parse_error_pretty_print(&error, &locator, &arena);
    //
    //         for (isize j = 0; j < parts.size; j++) {
    //             std::cout << parts[j];
    //         }
    //         std::cout << std::endl;
    //     }
    //     core_assert(false);
    // }

    semantic_analysis(file, &arena);
    CodeUnit code_unit = ast_compile_to_bytecode(&file->ast, &arena);

    for (isize i = 0; i < code_unit.functions.size; i++) {
        Slice<Inst> function = code_unit.functions[i];
        for (isize j = 0; j < function.size; j++) {
            std::cerr << function[j] << std::endl;
        }
        std::cerr << std::endl;
    }

    Arena exec_arena = {};
    arena_init(&exec_arena, 128 * 1024);
    defer(arena_free(&exec_arena));
    // defer({
    //     std::cerr << "Program memory used: " << arena_get_size(&exec_arena)
    //               << " bytes" << std::endl;
    // });

    VM* vm = vm_make(code_unit, 8 * 1024 * 1024, &exec_arena);
    vm->stdout = stdout_file;
    vm->stderr = stderr_file;
    while (true) {
        Inst inst = vm->code.functions[vm->fp][vm->ip];
        bool did_work = vm_execute_inst(vm);
        if (!did_work) {
            // The top value on the stack is the exit code
            u8 exit_code = *stack_pop<u8>(&vm->stack);
            return exit_code;
        }
    }
}

TEST(e2e, EmptyMain) {
    FILE* stdout_file = tmpfile();
    FILE* stderr_file = tmpfile();
    const char* source = R"SOURCE(
        main :: fn() {
            
        }
    )SOURCE";
    u8 exit_code = execute_to_end(source, stdout_file, stderr_file);
    EXPECT_EQ(exit_code, 0);
    EXPECT_EQ(ftell(stdout_file), 0);
    EXPECT_EQ(ftell(stderr_file), 0);
}

TEST(e2e, Constants) {
    FILE* stdout_file = tmpfile();
    FILE* stderr_file = tmpfile();
    const char* source = R"SOURCE(
        thing :: 13

        main :: fn() {

        }
    )SOURCE";
    u8 exit_code = execute_to_end(source, stdout_file, stderr_file);
    EXPECT_EQ(exit_code, 0);
    EXPECT_EQ(ftell(stdout_file), 0);
    EXPECT_EQ(ftell(stderr_file), 0);
}

TEST(e2e, Declaration) {
    FILE* stdout_file = tmpfile();
    FILE* stderr_file = tmpfile();
    const char* source = R"SOURCE(
        thing :: 13

        main :: fn() {
            a := 3
            b := true
        }
    )SOURCE";
    u8 exit_code = execute_to_end(source, stdout_file, stderr_file);
    EXPECT_EQ(exit_code, 0);
    EXPECT_EQ(ftell(stdout_file), 0);
    EXPECT_EQ(ftell(stderr_file), 0);
}

TEST(e2e, SimpleReturn) {
    FILE* stdout_file = tmpfile();
    FILE* stderr_file = tmpfile();
    const char* source = R"SOURCE(
        other :: fn() {
            return 1
        }

        main :: fn() {
            other()
        }
    )SOURCE";
    u8 exit_code = execute_to_end(source, stdout_file, stderr_file);
    EXPECT_EQ(exit_code, 0);
    EXPECT_EQ(ftell(stdout_file), 0);
    EXPECT_EQ(ftell(stderr_file), 0);
}

TEST(e2e, SimpleAddition) {
    FILE* stdout_file = tmpfile();
    FILE* stderr_file = tmpfile();
    const char* source = R"SOURCE(
        thing :: 13

        main :: fn() {
            a := 3 + 2
        }
    )SOURCE";
    u8 exit_code = execute_to_end(source, stdout_file, stderr_file);
    EXPECT_EQ(exit_code, 0);
    EXPECT_EQ(ftell(stdout_file), 0);
    EXPECT_EQ(ftell(stderr_file), 0);
}

TEST(e2e, SimpleAdditionReturn) {
    FILE* stdout_file = tmpfile();
    FILE* stderr_file = tmpfile();
    const char* source = R"SOURCE(
        calc :: fn() {
            a := 3 + 2

            return a
        }

        main :: fn() {
            a := calc()
        }
    )SOURCE";
    u8 exit_code = execute_to_end(source, stdout_file, stderr_file);
    EXPECT_EQ(exit_code, 0);
    EXPECT_EQ(ftell(stdout_file), 0);
    EXPECT_EQ(ftell(stderr_file), 0);
}

TEST(e2e, ComplexIntBinaryExpression) {
    FILE* stdout_file = tmpfile();
    FILE* stderr_file = tmpfile();
    const char* source = R"SOURCE(
        thing :: 13

        calc :: fn() {
            a := 3 + 2 * (3 - 1) * 3 * (7 + 16 - 9 / (3 + 1))

            return a
        }

        main :: fn() {
            calc()
        }
    )SOURCE";
    u8 exit_code = execute_to_end(source, stdout_file, stderr_file);
    EXPECT_EQ(exit_code, 0);
    EXPECT_EQ(ftell(stdout_file), 0);
    EXPECT_EQ(ftell(stderr_file), 0);
}
