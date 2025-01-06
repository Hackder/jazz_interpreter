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
    core_assert(i32_ptr != nullptr);

    i32* i32_ptr2 = arena_alloc<i32>(&arena, 2);
    core_assert(i32_ptr2 != nullptr);

    arena_free(&arena);

    EXPECT_EQ(arena.current, nullptr);
}

TEST(Core, RingBuffer) {
    Arena arena;
    arena_init(&arena, 10);
    defer(arena_free(&arena));

    RingBuffer<i32> ring_buffer;
    ring_buffer_init(&ring_buffer, 5, &arena);

    ring_buffer_push_end(&ring_buffer, 1);
    EXPECT_EQ(ring_buffer[0], 1);
    EXPECT_EQ(ring_buffer.size, 1);

    ring_buffer_push_end(&ring_buffer, 2);
    EXPECT_EQ(ring_buffer[0], 1);
    EXPECT_EQ(ring_buffer[1], 2);
    EXPECT_EQ(ring_buffer.size, 2);

    ring_buffer_pop_front(&ring_buffer);
    EXPECT_EQ(ring_buffer[0], 2);
    EXPECT_EQ(ring_buffer.size, 1);

    ring_buffer_pop_front(&ring_buffer);
    EXPECT_EQ(ring_buffer.size, 0);
}

TEST(Core, RingBufferWraparound) {
    Arena arena;
    arena_init(&arena, 10);
    defer(arena_free(&arena));

    RingBuffer<i32> ring_buffer;
    ring_buffer_init(&ring_buffer, 5, &arena);

    ring_buffer_push_end(&ring_buffer, 1);
    EXPECT_EQ(ring_buffer[0], 1);
    EXPECT_EQ(ring_buffer.size, 1);

    ring_buffer_push_end(&ring_buffer, 2);
    EXPECT_EQ(ring_buffer[1], 2);
    EXPECT_EQ(ring_buffer.size, 2);

    ring_buffer_push_end(&ring_buffer, 3);
    EXPECT_EQ(ring_buffer[2], 3);
    EXPECT_EQ(ring_buffer.size, 3);

    ring_buffer_push_end(&ring_buffer, 4);
    EXPECT_EQ(ring_buffer[3], 4);
    EXPECT_EQ(ring_buffer.size, 4);

    ring_buffer_push_end(&ring_buffer, 5);
    EXPECT_EQ(ring_buffer[4], 5);
    EXPECT_EQ(ring_buffer.size, 5);

    EXPECT_EQ(ring_buffer_pop_front(&ring_buffer), 1);
    EXPECT_EQ(ring_buffer_pop_front(&ring_buffer), 2);
    EXPECT_EQ(ring_buffer_pop_front(&ring_buffer), 3);
    EXPECT_EQ(ring_buffer.size, 2);
    EXPECT_EQ(ring_buffer[0], 4);

    ring_buffer_push_end(&ring_buffer, 6);
    EXPECT_EQ(ring_buffer[2], 6);
    EXPECT_EQ(ring_buffer.size, 3);

    ring_buffer_push_end(&ring_buffer, 7);
    EXPECT_EQ(ring_buffer[3], 7);
    EXPECT_EQ(ring_buffer.size, 4);

    ring_buffer_push_end(&ring_buffer, 8);
    EXPECT_EQ(ring_buffer[4], 8);
    EXPECT_EQ(ring_buffer.size, 5);
}

TEST(Core, RingBufferGrow) {
    Arena arena;
    arena_init(&arena, 10);
    defer(arena_free(&arena));

    RingBuffer<i32> ring_buffer;
    ring_buffer_init(&ring_buffer, 2, &arena);

    ring_buffer_push_end(&ring_buffer, 1);
    EXPECT_EQ(ring_buffer[0], 1);
    EXPECT_EQ(ring_buffer.size, 1);

    ring_buffer_push_end(&ring_buffer, 2);
    EXPECT_EQ(ring_buffer[1], 2);
    EXPECT_EQ(ring_buffer.size, 2);
    EXPECT_EQ(ring_buffer.capacity, 2);

    ring_buffer_push_end(&ring_buffer, 3);
    EXPECT_EQ(ring_buffer[2], 3);
    EXPECT_EQ(ring_buffer.size, 3);
    EXPECT_EQ(ring_buffer.capacity, 4);

    ring_buffer_push_end(&ring_buffer, 4);
    EXPECT_EQ(ring_buffer[3], 4);
    EXPECT_EQ(ring_buffer.size, 4);

    ring_buffer_push_end(&ring_buffer, 5);
    EXPECT_EQ(ring_buffer[4], 5);
    EXPECT_EQ(ring_buffer.size, 5);
    EXPECT_EQ(ring_buffer.capacity, 8);

    EXPECT_EQ(ring_buffer_pop_front(&ring_buffer), 1);
    EXPECT_EQ(ring_buffer_pop_front(&ring_buffer), 2);
    EXPECT_EQ(ring_buffer_pop_front(&ring_buffer), 3);
    EXPECT_EQ(ring_buffer.size, 2);
    EXPECT_EQ(ring_buffer[0], 4);

    ring_buffer_push_end(&ring_buffer, 6);
    EXPECT_EQ(ring_buffer[2], 6);
    EXPECT_EQ(ring_buffer.size, 3);

    ring_buffer_push_end(&ring_buffer, 7);
    EXPECT_EQ(ring_buffer[3], 7);
    EXPECT_EQ(ring_buffer.size, 4);

    ring_buffer_push_end(&ring_buffer, 8);
    EXPECT_EQ(ring_buffer[4], 8);
    EXPECT_EQ(ring_buffer.size, 5);
    EXPECT_EQ(ring_buffer.capacity, 8);
}

TEST(Core, HashMapAll) {
    Arena arena;
    arena_init(&arena, 1024);
    defer(arena_free(&arena));

    HashMap<i32, i32> map;
    hash_map_init(&map, 2, &arena);

    hash_map_insert_or_set(&map, 1, 101);
    hash_map_insert_or_set(&map, 2, 102);
    hash_map_insert_or_set(&map, 3, 103);

    EXPECT_EQ(hash_map_must_get(&map, 1), 101);
    EXPECT_EQ(hash_map_must_get(&map, 2), 102);
    EXPECT_EQ(hash_map_must_get(&map, 3), 103);

    hash_map_insert_or_set(&map, 1, 104);
    EXPECT_EQ(hash_map_must_get(&map, 1), 104);

    hash_map_remove(&map, 1);
    EXPECT_EQ(map.backing_map->find(1), map.backing_map->end());
    EXPECT_EQ(hash_map_get_ptr(&map, 1), nullptr);

    EXPECT_LT(arena_get_size(&arena), 200);
}
