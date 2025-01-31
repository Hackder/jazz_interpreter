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

    // Float
    Float_Add,
    Float_Sub,
    Float_Mul,
    Float_Div,
    Float_Equal,
    Float_NotEqual,

    // Bool
    Bool_LessThan,
    Bool_LessEqual,
    Bool_GreaterThan,
    Bool_GreaterEqual,
    Bool_LogicalAnd,
    Bool_LogicalOr,
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

struct InstCall {
    isize fp;
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

struct InstExit {
    u8 code;
};

enum class InstType {
    BinaryOp,
    UnaryOp,
    Call,
    Return,
    Mov,
    PushStack,
    PopStack,
    Exit,
};

struct Inst {
    InstType type;
    union {
        InstBinaryOp binary;
        InstUnaryOp unary;
        InstCall call;
        InstReturn ret;
        InstMov mov;
        InstPushStack push_stack;
        InstPopStack pop_stack;
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

inline Inst inst_return() { return Inst{.type = InstType::Return, .ret = {}}; }

inline Inst inst_mov(MemPtr dest, MemPtr src, isize size) {
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
    case BinOperand::Bool_LessThan:
        os << "Bool_LessThan";
        break;
    case BinOperand::Bool_LessEqual:
        os << "Bool_LessEqual";
        break;
    case BinOperand::Bool_GreaterThan:
        os << "Bool_GreaterThan";
        break;
    case BinOperand::Bool_GreaterEqual:
        os << "Bool_GreaterEqual";
        break;
    case BinOperand::Bool_LogicalAnd:
        os << "Bool_LogicalAnd";
        break;
    case BinOperand::Bool_LogicalOr:
        os << "Bool_LogicalOr";
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
    case InstType::Exit: {
        os << "Exit(" << inst.exit.code << ")";
        break;
    }
    }

    return os;
}
