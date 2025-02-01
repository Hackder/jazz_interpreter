#include "bytecode.hpp"
#include "core.hpp"
#include "optimizer.hpp"
#include "sema.hpp"
#include <gtest/gtest.h>

TEST(Optimizer, PopPushStackCombination) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));

    Array<Inst> instructions = {};
    array_init(&instructions, 16, &arena);

    array_push(&instructions, inst_pop_stack(8));
    array_push(&instructions, inst_pop_stack(16));
    array_push(&instructions, inst_push_stack(4));
    array_push(&instructions, inst_pop_stack(8));
    array_push(&instructions, inst_push_stack(32));

    optimize(&instructions, &arena);

    EXPECT_EQ(instructions.size, 1);
    EXPECT_EQ(instructions[0].type, InstType::PushStack);
    EXPECT_EQ(instructions[0].push_stack.size, 4);
}

TEST(Optimizer, PopPushStackCombinationWithJumps) {
    Arena arena;
    arena_init(&arena, 2048);
    defer(arena_free(&arena));

    Array<Inst> instructions = {};
    array_init(&instructions, 16, &arena);

    array_push(&instructions, inst_pop_stack(8));
    array_push(&instructions, inst_pop_stack(16));
    array_push(&instructions, inst_jump(7));
    array_push(&instructions, inst_push_stack(4));
    array_push(&instructions, inst_pop_stack(8));
    array_push(&instructions, inst_jump_if(mem_ptr_invalid(), 0));
    array_push(&instructions, inst_push_stack(32));
    array_push(&instructions, inst_push_stack(4));
    array_push(&instructions, inst_pop_stack(8));

    optimize(&instructions, &arena);

    EXPECT_EQ(instructions.size, 6);

    EXPECT_EQ(instructions[0].type, InstType::PopStack);
    EXPECT_EQ(instructions[0].pop_stack.size, 24);

    EXPECT_EQ(instructions[1].type, InstType::Jump);
    EXPECT_EQ(instructions[1].jump.new_ip, 5);

    EXPECT_EQ(instructions[2].type, InstType::PopStack);
    EXPECT_EQ(instructions[2].pop_stack.size, 4);

    EXPECT_EQ(instructions[3].type, InstType::JumpIf);
    EXPECT_EQ(instructions[3].jump_if.new_ip, 0);

    EXPECT_EQ(instructions[4].type, InstType::PushStack);
    EXPECT_EQ(instructions[4].push_stack.size, 32);

    EXPECT_EQ(instructions[5].type, InstType::PopStack);
    EXPECT_EQ(instructions[5].pop_stack.size, 4);
}
