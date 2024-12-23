#include "core.hpp"
#include "parser.hpp"
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
            Array<String> parts =
                parse_error_pretty_print(&error, &locator, &arena);

            for (isize j = 0; j < parts.size; j++) {
                std::cout << parts[j];
            }
            return 1;
        }
        return 1;
    }

    for (isize i = 0; i < file->ast->declarations.size; i++) {
        AstNode* node = file->ast->declarations[i];
        std::cout << ast_serialize_debug(node, &arena) << std::endl
                  << std::endl;
    }

    return 0;
}
