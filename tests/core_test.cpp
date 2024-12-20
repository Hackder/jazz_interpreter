#include "core.hpp"
#include <gtest/gtest.h>

TEST(Core, ArenaInit) {
    Arena arena;
    arena_init(&arena, 4 * 1024);
    defer(arena_free(&arena));

    EXPECT_EQ(arena.current->size, 0);
    EXPECT_EQ(arena.current->capacity, 4 * 1024);
    EXPECT_EQ(arena.current->prev, nullptr);
    EXPECT_EQ(arena.block_size_min, 4 * 1024);
}

TEST(Core, ArenaAlloc) {
    Arena arena;
    arena_init(&arena, 4 * 1024);
    defer(arena_free(&arena));

    i32* i32_ptr = arena_alloc<i32>(&arena);
    *i32_ptr = 42;

    EXPECT_EQ(arena.current->size, sizeof(i32));
    EXPECT_EQ(*i32_ptr, 42);

    i32* i32_ptr2 = arena_alloc<i32>(&arena);
    *i32_ptr2 = 43;

    EXPECT_EQ(arena.current->size, 2 * sizeof(i32));
    EXPECT_EQ(*i32_ptr2, 43);

    EXPECT_NE(i32_ptr, i32_ptr2);
}

TEST(Core, ArenaAllocGrow) {
    Arena arena;
    arena_init(&arena, 10);
    defer(arena_free(&arena));

    i32* i32_ptr = arena_alloc<i32>(&arena, 2);
    EXPECT_EQ(arena.current->size, 2 * sizeof(i32));

    i32* i32_ptr2 = arena_alloc<i32>(&arena, 2);
    EXPECT_EQ(arena.current->size, 2 * sizeof(i32));

    EXPECT_NE(i32_ptr, i32_ptr2);
    EXPECT_NE(arena.current->prev, nullptr);
}

TEST(Core, ArenaAllocFree) {
    Arena arena;
    arena_init(&arena, 10);
    i32* i32_ptr = arena_alloc<i32>(&arena, 2);
    assert(i32_ptr != nullptr);

    i32* i32_ptr2 = arena_alloc<i32>(&arena, 2);
    assert(i32_ptr2 != nullptr);

    arena_free(&arena);

    EXPECT_EQ(arena.current, nullptr);
}
