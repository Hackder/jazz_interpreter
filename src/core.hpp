/// A simple core library for C++
/// It implements custom allocators and better primitives
/// for string handling, which take advantage of these allocators

#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdio>
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
/// Assert
/// ------------------

#ifndef LIBFUZZER
#define core_debug_trap() __builtin_trap()
#else
#define core_debug_trap() std::abort()
#endif

#ifndef core_assert_msg
#define core_assert_msg(cond, msg, ...)                                        \
    do {                                                                       \
        if (!(cond)) {                                                         \
            core_assert_handler("Assertion Failure", #cond, __FILE__,          \
                                (i64)__LINE__, msg, ##__VA_ARGS__);            \
            core_debug_trap();                                                 \
        }                                                                      \
    } while (0)
#endif

#ifndef core_assert
#define core_assert(cond) core_assert_msg(cond, NULL)
#endif

inline void core_assert_handler(char const* prefix, char const* condition,
                                char const* file, i32 line, const char* msg,
                                ...) {
    fprintf(stderr, "%s(%d): %s: ", file, line, prefix);
    if (condition)
        fprintf(stderr, "`%s` ", condition);
    if (msg) {
        va_list va;
        va_start(va, msg);
        vfprintf(stderr, msg, va);
        va_end(va);
    }
    fprintf(stderr, "\n");
}

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

    isize new_capacity = std::max(arena->block_size_min, alloc_size);
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

inline isize arena_get_size(Arena* arena) {
    isize size = 0;
    MemoryBlock* block = arena->current;
    while (block) {
        size += block->size;
        block = block->prev;
    }

    return size;
}

/// ------------------
/// Strings
/// ------------------

struct String {
    const char* data = nullptr;
    isize size = 0;

    inline char operator[](isize index) {
        core_assert_msg(index >= 0, "%ld < 0", index);
        core_assert_msg(index < size, "%ld >= %ld", index, size);
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

inline String string_from_cstr_alloc(const char* cstr, Arena* arena) {
    isize size = (isize)strlen(cstr);
    char* data = arena_alloc<char>(arena, size + 1);
    memcpy(data, cstr, size);
    data[size] = '\0';

    return String{.data = data, .size = size};
}

inline String string_substr(String str, isize start, isize count) {
    core_assert_msg(start >= 0, "%ld < 0", start);
    core_assert_msg(start + count <= str.size, "%ld + %ld > %ld", start, count,
                    str.size);

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
        core_assert(index >= 0);
        core_assert(index < this->size);
        return data[index];
    }

    const T& operator[](isize index) const {
        core_assert(index >= 0);
        core_assert(index < this->size);
        return data[index];
    }
};

template <typename T>
inline void array_init(Array<T>* array, isize initial_capacity, Arena* arena) {
    core_assert_msg(initial_capacity > 0, "%ld <= 0", initial_capacity);
    array->arena = arena;
    array->data = arena_alloc<T>(arena, initial_capacity);
    array->size = 0;
    array->capacity = initial_capacity;
}

template <typename T>
inline Array<T>* array_make(isize initial_capacity, Arena* arena) {
    core_assert_msg(initial_capacity > 0, "%ld <= 0", initial_capacity);
    Array<T>* array = arena_alloc<Array<T>>(arena);
    array_init(array, initial_capacity, arena);

    return array;
}

template <typename T> inline void array_push(Array<T>* array, T value) {
    core_assert_msg(array->size <= array->capacity, "%ld > %ld", array->size,
                    array->capacity);
    core_assert_msg(array->data, "array->data is null");
    core_assert_msg(array->arena, "array->arena is null");
    core_assert_msg(array->capacity > 0, "%ld <= 0", array->capacity);

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

/// ------------------
/// Ring buffer
/// ------------------

template <typename T> struct RingBuffer {
    Arena* arena;
    T* data;
    isize capacity;
    isize size;
    // Head is the first element
    isize head;
    // Tail is one past the last element
    isize tail;

    T& operator[](isize index) {
        core_assert(index >= 0);
        core_assert(index < this->size);

        isize real_index = (head + index) % capacity;
        return this->data[real_index];
    }

    const T& operator[](isize index) const {
        core_assert(index >= 0);
        core_assert(index < this->size);

        isize real_index = (head + index) % capacity;
        return this->data[real_index];
    }
};

template <typename T>
inline void ring_buffer_init(RingBuffer<T>* ring_buffer, isize capacity,
                             Arena* arena) {
    ring_buffer->arena = arena;
    ring_buffer->data = arena_alloc<T>(arena, capacity);
    ring_buffer->capacity = capacity;
    ring_buffer->size = 0;
    ring_buffer->head = 0;
    ring_buffer->tail = 0;
}

template <typename T>
inline void ring_buffer_push_end(RingBuffer<T>* ring_buffer, T value) {
    core_assert(ring_buffer->tail <= ring_buffer->capacity);
    core_assert(ring_buffer->tail >= 0);
    core_assert(ring_buffer->head < ring_buffer->capacity);
    core_assert(ring_buffer->head >= 0);
    core_assert(ring_buffer->capacity > 0);
    core_assert(ring_buffer->data);

    // Grow
    if (ring_buffer->size == ring_buffer->capacity) {
        isize new_capacity = ring_buffer->capacity * 2;
        T* new_data = arena_alloc<T>(ring_buffer->arena, new_capacity);

        isize head = ring_buffer->head;
        isize tail = ring_buffer->tail;

        if (head < tail) {
            memcpy(new_data, ring_buffer->data + head,
                   sizeof(T) * (tail - head));
        } else {
            isize first_size = ring_buffer->capacity - head;
            memcpy(new_data, ring_buffer->data + head, sizeof(T) * first_size);
            memcpy(new_data + first_size, ring_buffer->data, sizeof(T) * tail);
        }

        ring_buffer->data = new_data;
        ring_buffer->capacity = new_capacity;
        ring_buffer->head = 0;
        ring_buffer->tail = ring_buffer->size;
    }

    ring_buffer->size += 1;
    if (ring_buffer->tail == ring_buffer->capacity) {
        ring_buffer->tail = 1;
        ring_buffer->data[0] = value;
        return;
    }

    ring_buffer->data[ring_buffer->tail] = value;
    ring_buffer->tail += 1;
}

template <typename T>
inline T ring_buffer_pop_front(RingBuffer<T>* ring_buffer) {
    core_assert(ring_buffer->tail <= ring_buffer->capacity);
    core_assert(ring_buffer->tail >= 0);
    core_assert(ring_buffer->head < ring_buffer->capacity);
    core_assert(ring_buffer->head >= 0);
    core_assert(ring_buffer->capacity > 0);
    core_assert(ring_buffer->data);

    core_assert_msg(ring_buffer->size > 0, "ring_buffer is empty");

    T value = ring_buffer->data[ring_buffer->head];

    ring_buffer->head = (ring_buffer->head + 1) % ring_buffer->capacity;
    ring_buffer->size -= 1;

    return value;
}
