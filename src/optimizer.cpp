#include "optimizer.hpp"
#include "bytecode.hpp"
#include "core.hpp"
#include <iostream>

void combine_stack_pop_push_instructions(Array<Inst>* instructions,
                                         Arena* arena) {
    Array<Inst> new_instructions = {};
    array_init(&new_instructions, instructions->size / 2, arena);

    Array<isize> jmp_instruction_indices = {};
    array_init(&jmp_instruction_indices, 4, arena);

    Array<isize> old_new_inst_mapping = {};
    array_init(&old_new_inst_mapping, instructions->size, arena);

    Array<isize> jump_targets = {};
    array_init(&jump_targets, 4, arena);
    for (isize i = instructions->size - 1; i >= 0; i--) {
        Inst inst = (*instructions)[i];
        if (inst.type == InstType::Jump) {
            array_push(&jump_targets, inst.jump.new_ip);
        } else if (inst.type == InstType::JumpIf) {
            array_push(&jump_targets, inst.jump_if.new_ip);
        }
    }

    for (isize i = 0; i < instructions->size; i++) {
        Inst inst = (*instructions)[i];

        switch (inst.type) {
        case InstType::JumpIf:
        case InstType::Jump: {
            array_push(&jmp_instruction_indices, new_instructions.size);
            array_push(&new_instructions, inst);
            break;
        }
        case InstType::PopStack:
        case InstType::PushStack: {
            isize total_push = 0;
            if (inst.type == InstType::PushStack) {
                total_push = inst.push_stack.size;
            } else if (inst.type == InstType::PopStack) {
                total_push = -inst.pop_stack.size;
            } else {
                core_assert(false);
            }

            for (i++; i < instructions->size; i++) {
                // If we are jumping to this position we can't combine the
                // instructions
                bool is_jump_target = array_contains(&jump_targets, i);
                if (is_jump_target) {
                    i -= 1;
                    break;
                }

                Inst next_inst = (*instructions)[i];
                if (next_inst.type == InstType::PushStack) {
                    total_push += next_inst.push_stack.size;
                    continue;
                } else if (next_inst.type == InstType::PopStack) {
                    total_push -= next_inst.pop_stack.size;
                    continue;
                }
                i -= 1;
                break;
            }

            if (total_push > 0) {
                Inst new_inst = inst_push_stack(total_push);
                array_push(&new_instructions, new_inst);
            } else if (total_push < 0) {
                Inst new_inst = inst_pop_stack(-total_push);
                array_push(&new_instructions, new_inst);
            }

            break;
        }
        default: {
            array_push(&new_instructions, inst);
            break;
        }
        }

        while (old_new_inst_mapping.size < i + 1) {
            array_push(&old_new_inst_mapping, new_instructions.size - 1);
        }
    }

    // Fix the jump instructions
    for (isize i = 0; i < jmp_instruction_indices.size; i++) {
        isize jmp_idx = jmp_instruction_indices[i];
        Inst* jmp_inst = &new_instructions[jmp_idx];

        if (jmp_inst->type == InstType::JumpIf) {
            isize new_ip = jmp_inst->jump_if.new_ip;
            isize mapped_ip = old_new_inst_mapping[new_ip];
            jmp_inst->jump_if.new_ip = mapped_ip;
        } else if (jmp_inst->type == InstType::Jump) {
            isize new_ip = jmp_inst->jump.new_ip;
            isize mapped_ip = old_new_inst_mapping[new_ip];
            jmp_inst->jump.new_ip = mapped_ip;
        } else {
            core_assert(false);
        }
    }

    array_clear(instructions);
    array_clone_to(&new_instructions, instructions, arena);
}

void optimize(Array<Inst>* instructions, Arena* arena) {
    combine_stack_pop_push_instructions(instructions, arena);
}
