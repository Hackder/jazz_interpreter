#include "core.hpp"
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

    return String{buffer, file_size};
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

    return 0;
}
