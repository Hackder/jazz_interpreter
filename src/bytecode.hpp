#pragma once

#include "core.hpp"

enum class MemPtrType {
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

struct InstExit {
    u8 code;
};

enum class InstType {
    BinaryOp,
    UnaryOp,
    Call,
    Return,
    Exit,
};

struct Inst {
    InstType type;
    union {
        InstBinaryOp binary;
        InstUnaryOp unary;
        InstCall call;
        InstReturn ret;
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

inline Inst inst_exit(u8 code) {
    return Inst{.type = InstType::Exit, .exit = InstExit{.code = code}};
}

struct CodeUnit {
    Slice<u8> static_data;
    // By convention, the first function is the entry point
    Slice<Slice<Inst>> functions;
};
