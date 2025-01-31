#pragma once

#include "ast.hpp"
#include "core.hpp"
#include "vm.hpp"

void std_println_int(VM* vm);

using BuiltinFunctionPtr = void (*)(VM*);

struct BuiltinFunction {
    String name;
    BuiltinFunctionPtr ptr;
    FunctionType type;
};

Slice<BuiltinFunction> builtin_functions(Arena* arena);

inline const char* builtin_function_name(BuiltinFunctionPtr function_ptr) {
    if (function_ptr == std_println_int) {
        return "std_print_int";
    }

    core_assert_msg(false, "Unknown builtin function");
    return nullptr;
}
