#pragma once

#include "bytecode.hpp"
#include "core.hpp"
#include <cstdio>

struct Stack {
    u8* data;
    isize size;
    isize capacity;
};

template <typename T> inline void stack_push(Stack* stack, T value) {
    core_assert(stack->size + (isize)sizeof(T) <= stack->capacity);
    memcpy(stack->data + stack->size, &value, sizeof(T));
    stack->size += sizeof(T);
}

template <typename T> inline T* stack_pop(Stack* stack) {
    core_assert(stack->size >= (isize)sizeof(T));
    stack->size -= sizeof(T);
    return (T*)(stack->data + stack->size);
}

struct VM {
    CodeUnit code;

    // Function pointer - points to the current function being executed
    isize fp;
    // Instruction pointer - points to the current instruction being executed
    isize ip;
    // Base pointer - points to the base of the current stack frame
    isize bp;

    FILE* stdout;
    FILE* stderr;

    Stack stack;
    // TODO(juraj): heap?
};

inline void vm_init(VM* vm, CodeUnit code, isize stack_size, Arena* arena) {
    vm->code = code;
    vm->fp = 0;
    vm->ip = 0;
    vm->bp = 0;
    vm->stdout = stdout;
    vm->stderr = stderr;
    vm->stack.data = arena_alloc<u8>(arena, stack_size);
    vm->stack.size = 0;
    vm->stack.capacity = stack_size;
}

inline VM* vm_make(CodeUnit code, isize stack_size, Arena* arena) {
    VM* vm = arena_alloc<VM>(arena);
    vm_init(vm, code, stack_size, arena);
    return vm;
}

template <typename T> inline T* vm_ptr_read(VM* vm, MemPtr ptr) {
    switch (ptr.type) {
    case MemPtrType::Invalid: {
        core_assert_msg(false,
                        "Null pointer dereference. This should never happen");
    }
    case MemPtrType::StackAbs: {
        core_assert(ptr.mem_offset + (isize)sizeof(T) <= vm->stack.size);
        return (T*)(vm->stack.data + ptr.mem_offset);
        break;
    }
    case MemPtrType::StackRel: {
        core_assert(ptr.mem_offset + vm->bp + (isize)sizeof(T) <=
                    vm->stack.size);
        return (T*)(vm->stack.data + vm->bp + ptr.mem_offset);
        break;
    }
    case MemPtrType::Heap: {
        // TODO(juraj): heap?
        core_assert(false);
        break;
    }
    case MemPtrType::StaticData: {
        core_assert(ptr.mem_offset + (isize)sizeof(T) <=
                    vm->code.static_data.size);
        return (T*)(vm->code.static_data.data + ptr.mem_offset);
        break;
    }
    }
}

template <typename T> inline void vm_ptr_write(VM* vm, MemPtr ptr, T value) {
    switch (ptr.type) {
    case MemPtrType::Invalid: {
        core_assert_msg(false,
                        "Null pointer dereference. This should never happen");
    }
    case MemPtrType::StackAbs: {
        core_assert(ptr.mem_offset + (isize)sizeof(T) <= vm->stack.capacity);
        memcpy(vm->stack.data + ptr.mem_offset, &value, sizeof(T));
        break;
    }
    case MemPtrType::StackRel: {
        core_assert(vm->bp + ptr.mem_offset + (isize)sizeof(T) <=
                    vm->stack.capacity);
        memcpy(vm->stack.data + vm->bp + ptr.mem_offset, &value, sizeof(T));
        break;
    }
    case MemPtrType::Heap: {
        // TODO(juraj): heap?
        core_assert(false);
        break;
    }
    case MemPtrType::StaticData: {
        core_assert_msg(
            false, "Cannot write to static data, this should never happen");
        break;
    }
    }
}

bool vm_execute_inst(VM* vm);
