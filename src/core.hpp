/// A simple core library for C++
/// It implements custom allocators and better primitives
/// for string handling, which take advantage of these allocators

#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <ostream>
#include <stdint.h>

/// ------------------
/// Defer
/// source: https://www.gingerbill.org/article/2015/08/19/defer-in-cpp/
/// ------------------

template <typename F> struct privDefer {
    F f;
    privDefer(F f) : f(f) {}
    ~privDefer() { f(); }
};

template <typename F> privDefer<F> defer_func(F f) { return privDefer<F>(f); }

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x) DEFER_2(x, __COUNTER__)
#define defer(code) auto DEFER_3(_defer_) = defer_func([&]() { code; })

/// ------------------
/// Type aliasses
/// ------------------

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;
using usize = size_t;
using isize = ptrdiff_t;

/// ------------------
/// Memory allocation
/// ------------------

struct MemoryBlock {
    u8* data;
    isize size;
    isize capacity;
    MemoryBlock* prev;
};

inline MemoryBlock* memory_block_create(isize size) {
    MemoryBlock* block = (MemoryBlock*)malloc(sizeof(MemoryBlock) + size);
    block->data = (u8*)(block + 1);
    block->size = 0;
    block->capacity = size;
    block->prev = nullptr;

    return block;
}

struct Arena {
    MemoryBlock* current;
    isize block_size_min;
};

inline void arena_init(Arena* arena, isize block_size_min) {
    arena->block_size_min = block_size_min;

    MemoryBlock* block = memory_block_create(block_size_min);
    arena->current = block;
}

template <typename T> inline T* arena_alloc(Arena* arena, isize count = 1) {
    u8* result = arena->current->data + arena->current->size;

    // Align forward to minimum alignment
    isize alignment = alignof(T);
    result = (u8*)((isize)(result + alignment - 1) & ~(alignment - 1));

    isize alloc_size = sizeof(T) * count;
    isize new_size = (result + alloc_size) - arena->current->data;

    if (new_size <= arena->current->capacity) {
        arena->current->size = new_size;
        memset(result, 0, alloc_size);
        return (T*)result;
    }

    isize new_capacity = std::max(arena->block_size_min, new_size);
    MemoryBlock* new_block = memory_block_create(new_capacity);

    new_block->prev = arena->current;
    arena->current = new_block;
    arena->current->size = alloc_size;

    memset(new_block->data, 0, alloc_size);
    return (T*)new_block->data;
}

inline void arena_free(Arena* arena) {
    MemoryBlock* block = arena->current;
    while (block) {
        MemoryBlock* prev = block->prev;
        free(block);
        block = prev;
    }

    arena->current = nullptr;
}

/// ------------------
/// Strings
/// ------------------

struct String {
    const char* data = nullptr;
    isize size = 0;

    inline char operator[](isize index) {
        assert(index >= 0);
        assert(index < size);
        return data[index];
    }
};

inline bool operator==(String a, String b) {
    if (a.size != b.size) {
        return false;
    }

    if (a.data == b.data) {
        return true;
    }

    return memcmp(a.data, b.data, a.size) == 0;
}

inline bool operator!=(String a, String b) { return !(a == b); }

inline bool operator==(String a, const char* b) {
    isize b_size = (isize)strlen(b);
    if (a.size != b_size) {
        return false;
    }

    return memcmp(a.data, b, a.size) == 0;
}

inline bool operator!=(String a, const char* b) { return !(a == b); }

// Only used to allow gtest to print strings
inline std::ostream& operator<<(std::ostream& os, String str) {
    for (isize i = 0; i < str.size; i++) {
        os << str.data[i];
    }

    return os;
}

inline String string_from_cstr(const char* cstr) {
    return String{.data = cstr, .size = (isize)strlen(cstr)};
}

inline String string_substr(String str, isize start, isize count) {
    assert(start >= 0);
    assert(start + count <= str.size);

    return String{str.data + start, count};
}

/// ------------------
/// Array
/// ------------------

template <typename T> struct Array {
    Arena* arena;
    T* data;
    isize size;
    isize capacity;

    T& operator[](isize index) {
        assert(index >= 0);
        assert(index < size);
        return data[index];
    }

    const T& operator[](isize index) const {
        assert(index >= 0);
        assert(index < size);
        return data[index];
    }
};

template <typename T>
inline Array<T> array_make(isize initial_capacity, Arena* arena) {
    Array<T> array = {};
    array->arena = arena;
    array->data = arena_alloc<T>(arena, initial_capacity);
    array->size = 0;
    array->capacity = initial_capacity;
}

template <typename T> inline void array_push(Array<T>* array, T value) {
    assert(array->size <= array->capacity);

    if (array->size == array->capacity) {
        isize new_capacity = array->capacity * 2;
        T* new_data = arena_alloc<T>(array->arena, new_capacity);
        memcpy(new_data, array->data, sizeof(T) * array->size);
        array->data = new_data;
        array->capacity = new_capacity;
    }

    array->data[array->size] = value;
    array->size += 1;
}
