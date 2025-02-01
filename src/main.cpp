#include "compiler.hpp"
#include "core.hpp"
#include "parser.hpp"
#include "sema.hpp"
#include "vm.hpp"
#include <cstdio>
#include <iostream>

String read_file(Arena* arena, const char* file_name) {
    FILE* file = fopen(file_name, "r");
    defer(fclose(file));

    if (!file) {
        std::cerr << "Error: Could not open file " << file_name << std::endl;
        return {};
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        std::cerr << "Error: Could not seek file " << file_name << std::endl;
        return {};
    }
    isize file_size = ftell(file);
    if (file_size < 0) {
        std::cerr << "Error: Could not get file size of " << file_name
                  << std::endl;
        return {};
    }
    rewind(file);

    char* buffer = arena_alloc<char>(arena, file_size + 1);
    isize read_size = fread(buffer, 1, file_size, file);
    if (read_size != file_size) {
        std::cerr << "Error: Could not read file " << file_name << std::endl;
        return {};
    }
    buffer[file_size] = '\0';

    return String{.data = buffer, .size = file_size};
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file.jazz>" << std::endl;
        return 1;
    }

    Arena arena;
    arena_init(&arena, 16 * 1024);
    defer(arena_free(&arena));
    defer({
        std::cerr << "Memory used: " << arena_get_size(&arena) << " bytes"
                  << std::endl;
    });

    const char* source_file = argv[1];
    String source_code = read_file(&arena, source_file);

    Tokenizer tokenizer;
    tokenizer_init(&tokenizer, source_code);
    AstFile* file = ast_file_make(tokenizer, 16, &arena);
    ast_file_parse(file, &arena);

    if (file->errors.size > 0) {
        TokenLocator locator;
        token_locator_init(&locator, source_code, &arena);

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
        return 1;
    }

    semantic_analysis(file, &arena);

    CodeUnit code_unit = ast_compile_to_bytecode(&file->ast, true, &arena);

    for (isize i = 0; i < code_unit.functions.size; i++) {
        Slice<Inst> function = code_unit.functions[i];
        for (isize j = 0; j < function.size; j++) {
            std::cerr << j << ": " << function[j] << std::endl;
        }
        std::cerr << std::endl;
    }

    Arena exec_arena = {};
    arena_init(&exec_arena, 128 * 1024);
    defer(arena_free(&exec_arena));
    defer({
        std::cerr << "Program memory used: " << arena_get_size(&exec_arena)
                  << " bytes" << std::endl;
    });

    VM* vm = vm_make(code_unit, 8 * 1024 * 1024, &exec_arena);
    isize instructions_executed = 0;
    defer({
        std::cerr << "Instructions executed: " << instructions_executed
                  << std::endl;
    });

    while (true) {
        bool did_work = vm_execute_inst(vm);
        instructions_executed += 1;
        if (!did_work) {
            // The top value on the stack is the exit code
            u8 exit_code = stack_pop<u8>(&vm->stack);
            return exit_code;
        }
    }
}
