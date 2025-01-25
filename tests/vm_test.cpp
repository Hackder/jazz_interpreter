#include "bytecode.hpp"
#include "core.hpp"
#include "vm.hpp"
#include <gtest/gtest.h>

TEST(VM, ExecuteInstruction) {
    Arena arena;
    arena_init(&arena, 4024);
    defer(arena_free(&arena));

    Inst instructions[] = {
        {.type = InstType::BinaryOp,
         .binary = {BinOperand::Int_Add,
                    {MemPtrType::StackAbs, 0},
                    {MemPtrType::StackAbs, 8},
                    {MemPtrType::StackAbs, 16}}},
    };

    Array<Slice<Inst>> functions = {};
    array_init(&functions, 1, &arena);
    array_push(&functions, slice_from_inline_alloc(instructions, &arena));
    Array<u8> static_data = {};

    CodeUnit code = {
        .static_data = slice_from_array(&static_data),
        .functions = slice_from_array(&functions),
    };

    VM* vm = vm_make(code, 1024, &arena);
    stack_push<i64>(&vm->stack, 10);
    stack_push<i64>(&vm->stack, 20);
    stack_push<i64>(&vm->stack, 30);

    EXPECT_EQ(*vm_ptr_read<i64>(vm, {MemPtrType::StackAbs, 0}), (isize)10);
    EXPECT_EQ(*vm_ptr_read<i64>(vm, {MemPtrType::StackAbs, 8}), (isize)20);
    EXPECT_EQ(*vm_ptr_read<i64>(vm, {MemPtrType::StackAbs, 16}), (isize)30);

    vm_execute_inst(vm);

    EXPECT_EQ(*vm_ptr_read<i64>(vm, {MemPtrType::StackAbs, 0}), (isize)50);
    EXPECT_EQ(*vm_ptr_read<i64>(vm, {MemPtrType::StackAbs, 8}), (isize)20);
    EXPECT_EQ(*vm_ptr_read<i64>(vm, {MemPtrType::StackAbs, 16}), (isize)30);
}
