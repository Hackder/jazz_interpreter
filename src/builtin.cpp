#include "builtin.hpp"
#include "ast.hpp"
#include "vm.hpp"

void std_println_int(VM* vm) {
    isize value = *stack_peek<isize>(&vm->stack);
    fprintf(vm->stdout, "%ld\n", value);
}

void std_print_int(VM* vm) {
    isize value = *stack_peek<isize>(&vm->stack);
    fprintf(vm->stdout, "%ld", value);
}

void std_print_space(VM* vm) { fprintf(vm->stdout, " "); }

void std_print_newline(VM* vm) { fprintf(vm->stdout, "\n"); }

Slice<BuiltinFunction> builtin_functions(Arena* arena) {
    Array<BuiltinFunction>* functions = array_make<BuiltinFunction>(1, arena);

    // -------------------
    // std_println_int
    // -------------------

    {
        Array<TypeSetHandle*>* parameters =
            array_make<TypeSetHandle*>(1, arena);
        array_push(parameters, type_set_make_with(Type::get_int(), arena));

        FunctionType* t = FunctionType::make(
            *parameters, type_set_make_with(Type::get_void(), arena), arena);

        BuiltinFunction print_int = {
            .name = string_from_cstr("std_println_int"),
            .ptr = std_println_int,
            .type = *t,
        };
        array_push(functions, print_int);
    }

    // -------------------
    // std_print_int
    // -------------------

    {
        Array<TypeSetHandle*>* parameters =
            array_make<TypeSetHandle*>(1, arena);
        array_push(parameters, type_set_make_with(Type::get_int(), arena));

        FunctionType* t = FunctionType::make(
            *parameters, type_set_make_with(Type::get_void(), arena), arena);

        BuiltinFunction print_int = {
            .name = string_from_cstr("std_print_int"),
            .ptr = std_print_int,
            .type = *t,
        };
        array_push(functions, print_int);
    }

    // -------------------
    // std_print_space
    // -------------------

    {
        Array<TypeSetHandle*>* parameters =
            array_make<TypeSetHandle*>(0, arena);

        FunctionType* t = FunctionType::make(
            *parameters, type_set_make_with(Type::get_void(), arena), arena);

        BuiltinFunction print_int = {
            .name = string_from_cstr("std_print_space"),
            .ptr = std_print_space,
            .type = *t,
        };
        array_push(functions, print_int);
    }

    // -------------------
    // std_print_newline
    // -------------------

    {
        Array<TypeSetHandle*>* parameters =
            array_make<TypeSetHandle*>(0, arena);

        FunctionType* t = FunctionType::make(
            *parameters, type_set_make_with(Type::get_void(), arena), arena);

        BuiltinFunction print_int = {
            .name = string_from_cstr("std_print_newline"),
            .ptr = std_print_newline,
            .type = *t,
        };
        array_push(functions, print_int);
    }

    return array_to_slice(functions);
}
