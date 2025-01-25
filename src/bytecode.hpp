#pragma once

#include "core.hpp"

enum class MemPtrType {
    StackAbs = 0,
    // A poiter relative to BP (this is useful for local variables and function
    // arguments)
    StackRel = 1,
    Heap = 2,
    StaticData = 3,
};

struct MemPtr {
    MemPtrType type : 2;
    isize mem_offset : 62;
};

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

enum class InstType {
    BinaryOp,
    UnaryOp,
    Call,
    Return,
};

struct Inst {
    InstType type;
    union {
        InstBinaryOp binary;
        InstUnaryOp unary;
        InstCall call;
    };
};

struct CodeUnit {
    Slice<u8> static_data;
    // By convention, the first function is the entry point
    Slice<Slice<Inst>> functions;
};
