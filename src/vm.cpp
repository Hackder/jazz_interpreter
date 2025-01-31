#include "vm.hpp"
#include "bytecode.hpp"
#include "core.hpp"

#define CASE_BINARY_OP(op, type, op_symbol)                                    \
    case BinOperand::op: {                                                     \
        type left = *vm_ptr_read<type>(vm, current_inst.binary.left);          \
        type right = *vm_ptr_read<type>(vm, current_inst.binary.right);        \
                                                                               \
        type result = left op_symbol right;                                    \
        vm_ptr_write<type>(vm, current_inst.binary.dest, result);              \
        break;                                                                 \
    }

bool vm_execute_inst(VM* vm) {
    Inst current_inst = vm->code.functions[vm->fp][vm->ip];
    vm->ip += 1;

    switch (current_inst.type) {
    case InstType::UnaryOp: {
        switch (current_inst.unary.op) {
        case UnaryOperand::Int_Negation: {
            i64 value = *vm_ptr_read<i64>(vm, current_inst.unary.operand);
            i64 result = -value;
            vm_ptr_write<i64>(vm, current_inst.unary.dest, result);
            break;
        }
        case UnaryOperand::Float_Negation: {
            f64 value = *vm_ptr_read<f64>(vm, current_inst.unary.operand);
            f64 result = -value;
            vm_ptr_write<f64>(vm, current_inst.unary.dest, result);
            break;
        }
        case UnaryOperand::Bool_Not: {
            bool value = *vm_ptr_read<bool>(vm, current_inst.unary.operand);
            bool result = !value;
            vm_ptr_write<bool>(vm, current_inst.unary.dest, result);
            break;
        }
        }
        break;
    }
    case InstType::BinaryOp: {
        switch (current_inst.binary.op) {
            CASE_BINARY_OP(Int_Add, i64, +)
            CASE_BINARY_OP(Int_Sub, i64, -)
            CASE_BINARY_OP(Int_Mul, i64, *)
            CASE_BINARY_OP(Int_Div, i64, /)
            CASE_BINARY_OP(Int_BinaryAnd, i64, &)
            CASE_BINARY_OP(Int_BinaryOr, i64, |)
            CASE_BINARY_OP(Int_Equal, i64, ==)
            CASE_BINARY_OP(Int_NotEqual, i64, !=)

            CASE_BINARY_OP(Float_Add, f64, +)
            CASE_BINARY_OP(Float_Sub, f64, -)
            CASE_BINARY_OP(Float_Mul, f64, *)
            CASE_BINARY_OP(Float_Div, f64, /)
            CASE_BINARY_OP(Float_Equal, f64, ==)
            CASE_BINARY_OP(Float_NotEqual, f64, !=)

            CASE_BINARY_OP(Bool_LessThan, bool, <)
            CASE_BINARY_OP(Bool_LessEqual, bool, <=)
            CASE_BINARY_OP(Bool_GreaterThan, bool, >)
            CASE_BINARY_OP(Bool_GreaterEqual, bool, >=)
            CASE_BINARY_OP(Bool_LogicalAnd, bool, &&)
            CASE_BINARY_OP(Bool_LogicalOr, bool, ||)
            CASE_BINARY_OP(Bool_Equal, bool, ==)
            CASE_BINARY_OP(Bool_NotEqual, bool, !=)
        }
        break;
    }
    case InstType::Call: {
        stack_push(&vm->stack, vm->fp);
        stack_push(&vm->stack, vm->ip);
        stack_push(&vm->stack, vm->bp);
        vm->fp = current_inst.call.fp;
        vm->ip = 0;
        vm->bp = vm->stack.size;
        break;
    }
    case InstType::Return: {
        vm->bp = *stack_pop<isize>(&vm->stack);
        vm->ip = *stack_pop<isize>(&vm->stack);
        vm->fp = *stack_pop<isize>(&vm->stack);
        break;
    }
    case InstType::Exit: {
        stack_push(&vm->stack, current_inst.exit.code);
        return false;
    }
    case InstType::Mov: {
        isize size = current_inst.mov.size;
        u8* src = vm_ptr_to_raw(vm, current_inst.mov.src);
        u8* dest = vm_ptr_to_raw(vm, current_inst.mov.dest);
        memcpy(dest, src, size);
        break;
    }
    case InstType::PushStack: {
        stack_push_size(&vm->stack, current_inst.push_stack.size);
        break;
    }
    case InstType::PopStack: {
        stack_pop_size(&vm->stack, current_inst.pop_stack.size);
        break;
    }
    }

    return true;
}
