#pragma once

#include "core.hpp"
#include <ostream>

enum class MemPtrType : u8 {
    Invalid = 0,
    StackAbs = 1,
    // A poiter relative to BP (this is useful for local variables and function
    // arguments)
    StackRel = 2,
    Heap = 3,
    StaticData = 4,
};

struct MemPtr {
    MemPtrType type : 3;
    isize mem_offset : 61;
};

inline MemPtr mem_ptr_invalid() {
    return MemPtr{.type = MemPtrType::Invalid, .mem_offset = 0};
};

inline MemPtr mem_ptr_stack_rel(isize offset) {
    return MemPtr{.type = MemPtrType::StackRel, .mem_offset = offset};
}

inline MemPtr mem_ptr_static_data(isize offset) {
    return MemPtr{.type = MemPtrType::StaticData, .mem_offset = offset};
}

enum class BinOperand {
    // Int
    Int_Add,
    Int_Sub,
    Int_Mul,
    Int_Div,
    Int_BinaryAnd,
    Int_BinaryOr,
    Int_Equal,
    Int_NotEqual,
    Int_LessThan,
    Int_LessEqual,
    Int_GreaterThan,
    Int_GreaterEqual,

    // Float
    Float_Add,
    Float_Sub,
    Float_Mul,
    Float_Div,
    Float_Equal,
    Float_NotEqual,
    Float_LessThan,
    Float_LessEqual,
    Float_GreaterThan,
    Float_GreaterEqual,

    // Bool
    Bool_Equal,
    Bool_NotEqual,
};

struct InstBinaryOp {
    BinOperand op;
    MemPtr dest;
    MemPtr left;
    MemPtr right;
};

enum class UnaryOperand {
    Int_Negation,
    Float_Negation,
    Bool_Not,
};

struct InstUnaryOp {
    UnaryOperand op;
    MemPtr dest;
    MemPtr operand;
};

const isize CALL_METADATA_SIZE = 3 * sizeof(isize);

struct InstCall {
    isize fp;
};

struct InstCallBuiltin {
    void* builtin;
};

struct InstReturn {};

struct InstMov {
    MemPtr dest;
    MemPtr src;
    isize size;
};

struct InstPushStack {
    isize size;
};

struct InstPopStack {
    isize size;
};

struct InstJumpIf {
    MemPtr condition;
    isize new_ip;
    bool expected;
};

struct InstJump {
    isize new_ip;
};

struct InstExit {
    u8 code;
};

enum class InstType {
    BinaryOp,
    UnaryOp,
    Call,
    CallBuiltin,
    Return,
    Mov,
    PushStack,
    PopStack,
    JumpIf,
    Jump,
    Exit,
};

struct Inst {
    InstType type;
    union {
        InstBinaryOp binary;
        InstUnaryOp unary;
        InstCall call;
        InstCallBuiltin call_builtin;
        InstReturn ret;
        InstMov mov;
        InstPushStack push_stack;
        InstPopStack pop_stack;
        InstJumpIf jump_if;
        InstJump jump;
        InstExit exit;
    };
};

inline Inst inst_binary_op(BinOperand op, MemPtr dest, MemPtr left,
                           MemPtr right) {
    return Inst{.type = InstType::BinaryOp,
                .binary = InstBinaryOp{
                    .op = op, .dest = dest, .left = left, .right = right}};
}

inline Inst inst_unary_op(UnaryOperand op, MemPtr dest, MemPtr operand) {
    return Inst{.type = InstType::UnaryOp,
                .unary =
                    InstUnaryOp{.op = op, .dest = dest, .operand = operand}};
}

inline Inst inst_call(isize fp) {
    return Inst{.type = InstType::Call, .call = InstCall{.fp = fp}};
}

inline Inst inst_call_builtin(void* builtin) {
    return Inst{.type = InstType::CallBuiltin,
                .call_builtin = InstCallBuiltin{.builtin = builtin}};
}

inline Inst inst_return() { return Inst{.type = InstType::Return, .ret = {}}; }

inline Inst inst_mov(MemPtr dest, MemPtr src, isize size) {
    core_assert(src.type != MemPtrType::Invalid);
    core_assert(dest.type != MemPtrType::Invalid);
    return Inst{.type = InstType::Mov,
                .mov = InstMov{.dest = dest, .src = src, .size = size}};
}

inline Inst inst_push_stack(isize size) {
    core_assert(size >= 0);
    return Inst{.type = InstType::PushStack, .push_stack = InstPushStack{size}};
}

inline Inst inst_pop_stack(isize size) {
    core_assert(size >= 0);
    return Inst{.type = InstType::PopStack, .pop_stack = InstPopStack{size}};
}

inline Inst inst_jump_if(MemPtr condition, isize new_ip) {
    return Inst{.type = InstType::JumpIf,
                .jump_if = InstJumpIf{.condition = condition,
                                      .new_ip = new_ip,
                                      .expected = true}};
}

inline Inst inst_jump_if_not(MemPtr condition, isize new_ip) {
    return Inst{.type = InstType::JumpIf,
                .jump_if = InstJumpIf{.condition = condition,
                                      .new_ip = new_ip,
                                      .expected = false}};
}

inline Inst inst_jump(isize new_ip) {
    return Inst{.type = InstType::Jump, .jump = InstJump{.new_ip = new_ip}};
}

inline Inst inst_exit(u8 code) {
    return Inst{.type = InstType::Exit, .exit = InstExit{.code = code}};
}

struct CodeUnit {
    Slice<u8> static_data;
    // By convention, the first function is the entry point
    Slice<Slice<Inst>> functions;
};

inline std::ostream& operator<<(std::ostream& os, MemPtr ptr) {
    switch (ptr.type) {
    case MemPtrType::Invalid:
        os << "(Invalid)";
        break;
    case MemPtrType::StackAbs:
        os << "(StackAbs " << ptr.mem_offset << ")";
        break;
    case MemPtrType::StackRel:
        os << "(StackRel " << ptr.mem_offset << ")";
        break;
    case MemPtrType::Heap:
        os << "(Heap " << ptr.mem_offset << ")";
        break;
    case MemPtrType::StaticData:
        os << "(StaticData " << ptr.mem_offset << ")";
        break;
    }

    return os;
}

inline std::ostream& operator<<(std::ostream& os, const BinOperand& op) {
    switch (op) {
    case BinOperand::Int_Add:
        os << "Int_Add";
        break;
    case BinOperand::Int_Sub:
        os << "Int_Sub";
        break;
    case BinOperand::Int_Mul:
        os << "Int_Mul";
        break;
    case BinOperand::Int_Div:
        os << "Int_Div";
        break;
    case BinOperand::Int_BinaryAnd:
        os << "Int_BinaryAnd";
        break;
    case BinOperand::Int_BinaryOr:
        os << "Int_BinaryOr";
        break;
    case BinOperand::Int_Equal:
        os << "Int_Equal";
        break;
    case BinOperand::Int_NotEqual:
        os << "Int_NotEqual";
        break;
    case BinOperand::Int_LessThan:
        os << "Int_LessThan";
        break;
    case BinOperand::Int_LessEqual:
        os << "Int_LessEqual";
        break;
    case BinOperand::Int_GreaterThan:
        os << "Int_GreaterThan";
        break;
    case BinOperand::Int_GreaterEqual:
        os << "Int_GreaterEqual";
        break;
    case BinOperand::Float_Add:
        os << "Float_Add";
        break;
    case BinOperand::Float_Sub:
        os << "Float_Sub";
        break;
    case BinOperand::Float_Mul:
        os << "Float_Mul";
        break;
    case BinOperand::Float_Div:
        os << "Float_Div";
        break;
    case BinOperand::Float_Equal:
        os << "Float_Equal";
        break;
    case BinOperand::Float_NotEqual:
        os << "Float_NotEqual";
        break;
    case BinOperand::Float_LessThan:
        os << "Float_LessThan";
        break;
    case BinOperand::Float_LessEqual:
        os << "Float_LessEqual";
        break;
    case BinOperand::Float_GreaterThan:
        os << "Float_GreaterThan";
        break;
    case BinOperand::Float_GreaterEqual:
        os << "Float_GreaterEqual";
        break;
    case BinOperand::Bool_Equal:
        os << "Bool_Equal";
        break;
    case BinOperand::Bool_NotEqual:
        os << "Bool_NotEqual";
        break;
    }
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const UnaryOperand& op) {
    switch (op) {
    case UnaryOperand::Int_Negation:
        os << "Int_Negation";
        break;
    case UnaryOperand::Float_Negation:
        os << "Float_Negation";
        break;
    case UnaryOperand::Bool_Not:
        os << "Bool_Not";
        break;
    }
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const Inst& inst) {
    switch (inst.type) {
    case InstType::BinaryOp: {
        os << "BinaryOp(" << inst.binary.op << " " << inst.binary.dest << " "
           << inst.binary.left << " " << inst.binary.right << ")";
        break;
    }
    case InstType::UnaryOp: {
        os << "UnaryOp(" << inst.unary.op << " " << inst.unary.dest << " "
           << inst.unary.operand << ")";
        break;
    }
    case InstType::Call: {
        os << "Call(" << inst.call.fp << ")";
        break;
    }
    case InstType::CallBuiltin: {
        os << "CallBuiltin(" << inst.call_builtin.builtin << ")";
        break;
    }
    case InstType::Return: {
        os << "Return";
        break;
    }
    case InstType::Mov: {
        os << "Mov(" << inst.mov.dest << " " << inst.mov.src << " "
           << inst.mov.size << ")";
        break;
    }
    case InstType::PushStack: {
        os << "PushStack(" << inst.push_stack.size << ")";
        break;
    }
    case InstType::PopStack: {
        os << "PopStack(" << inst.pop_stack.size << ")";
        break;
    }
    case InstType::JumpIf: {
        os << "JumpIf(" << inst.jump_if.condition << " " << inst.jump_if.new_ip
           << " " << inst.jump_if.expected << ")";
        break;
    }
    case InstType::Jump: {
        os << "Jump(" << inst.jump.new_ip << ")";
        break;
    }
    case InstType::Exit: {
        os << "Exit(" << inst.exit.code << ")";
        break;
    }
    }

    return os;
}
