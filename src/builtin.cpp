#include "builtin.hpp"
#include "ast.hpp"
#include "vm.hpp"

void std_println_int(VM* vm) {
    isize value = *stack_peek<isize>(&vm->stack);
    fprintf(vm->stdout, "%ld\n", value);
}

Slice<BuiltinFunction> builtin_functions(Arena* arena) {
    Array<BuiltinFunction>* functions = array_make<BuiltinFunction>(1, arena);

    // -------------------
    // std_print_int
    // -------------------

    Array<TypeSetHandle*>* parameters = array_make<TypeSetHandle*>(1, arena);
    array_push(parameters, type_set_make_with(Type::get_int(), arena));

    FunctionType* t = FunctionType::make(
        *parameters, type_set_make_with(Type::get_void(), arena), arena);

    BuiltinFunction print_int = {
        .name = string_from_cstr("std_println_int"),
        .ptr = std_println_int,
        .type = *t,
    };
    array_push(functions, print_int);

    return array_to_slice(functions);
}
