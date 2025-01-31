#pragma once

#include "bytecode.hpp"
#include "core.hpp"
#include "tokenizer.hpp"

// ------------------
// Type system
// ------------------

enum class TypeKind { Void, Integer, Float, String, Bool, Function };

struct AstNode;
struct FunctionType;
struct Type;
struct TypeSet;
struct TypeSetHandle;

struct Type {
    TypeKind kind;
    isize size;

    static Type* get_int() {
        static Type type = {TypeKind::Integer, 8};
        return &type;
    }

    static Type* get_float() {
        static Type type = {TypeKind::Float, 8};
        return &type;
    }

    static Type* get_string() {
        static Type type = {TypeKind::String, 8};
        return &type;
    }

    static Type* get_bool() {
        static Type type = {TypeKind::Bool, 1};
        return &type;
    }

    static Type* get_void() {
        static Type type = {TypeKind::Void, 0};
        return &type;
    }

    static Type* get_by_kind(TypeKind kind) {
        switch (kind) {
        case TypeKind::Void:
            return get_void();
        case TypeKind::Integer:
            return get_int();
        case TypeKind::Float:
            return get_float();
        case TypeKind::String:
            return get_string();
        case TypeKind::Bool:
            return get_bool();
        default:
            core_assert_msg(false, "Unsupported type kind");
            return nullptr;
        }
    }

    FunctionType* as_function();
};

struct TypeSet {
    Arena* arena;
    Array<Type*> types;
    bool is_full;
};

struct TypeSetHandle {
    TypeSet* set;
    Array<TypeSetHandle**> backreferences;
};

struct FunctionType : public Type {
    Array<TypeSetHandle*> parameters;
    TypeSetHandle* return_type;

    static FunctionType* make(Array<TypeSetHandle*> parameters,
                              TypeSetHandle* return_type, Arena* arena) {
        FunctionType* type = arena_alloc<FunctionType>(arena);
        type->kind = TypeKind::Function;
        type->parameters = parameters;
        type->size = 8;
        for (isize i = 0; i < parameters.size; i++) {
            array_push(&parameters[i]->backreferences, &type->parameters[i]);
        }
        type->return_type = return_type;
        array_push(&return_type->backreferences, &type->return_type);
        return type;
    }
};

inline FunctionType* Type::as_function() {
    core_assert(this->kind == TypeKind::Function);
    return static_cast<FunctionType*>(this);
}

inline bool function_type_intersect_with(FunctionType* a, FunctionType* b);

inline void type_set_init(TypeSet* set, isize capacity, Arena* arena) {
    core_assert(set);

    set->arena = arena;
    array_init(&set->types, capacity, arena);
    set->is_full = true;
}

inline TypeSetHandle* type_set_make(isize capacity, Arena* arena) {
    TypeSet* set = arena_alloc<TypeSet>(arena);
    type_set_init(set, capacity, arena);
    TypeSetHandle* handle = arena_alloc<TypeSetHandle>(arena);
    handle->set = set;
    array_init(&handle->backreferences, 1, arena);
    return handle;
}

inline TypeSetHandle* type_set_make_with(Type* type, Arena* arena) {
    TypeSetHandle* handle = type_set_make(0, arena);
    array_push(&handle->set->types, type);
    handle->set->is_full = false;
    return handle;
}

FunctionType* type_set_get_function(TypeSetHandle* handle);

void type_set_reassign_all(TypeSetHandle* handle, TypeSetHandle* other);

bool type_set_intersect_if_result(TypeSetHandle* handle, TypeSetHandle* other);

Type* type_set_get_single(TypeSetHandle* handle);

bool function_type_intersect_with(FunctionType* a, FunctionType* b);

template <isize N>
inline bool type_set_intersect_if_result_kinds(TypeSetHandle* handle,
                                               const TypeKind (&kinds)[N]) {
    core_assert(handle);

    if (handle->set->is_full) {
        handle->set->is_full = false;
        array_init(&handle->set->types, N, handle->set->arena);
        for (isize i = 0; i < N; i++) {
            array_push(&handle->set->types, Type::get_by_kind(kinds[i]));
        }
        return true;
    }

    isize original_size = handle->set->types.size;

    for (isize i = 0; i < handle->set->types.size; i++) {
        Type* type = handle->set->types[i];
        bool found = false;

        for (isize j = 0; j < N; j++) {
            if (type->kind == kinds[j]) {
                found = true;
                break;
            }
        }

        if (!found) {
            array_remove_at_unordered(&handle->set->types, i);
            i--;
        }
    }

    if (handle->set->types.size == 0) {
        // HACK(juraj): to allow us to report errors in a reasonable way,
        // we need to know the original sets. This is a hack to allow us to
        // restore the original set.
        handle->set->types.size = original_size;
        return false;
    }

    return true;
}

// ------------------
// AST
// ------------------

enum class AstNodeKind {
    Literal,
    Identifier,
    Binary,
    Unary,
    Call,
    If,
    For,
    Break,
    Continue,
    Return,
    Block,
    Parameter,
    Function,
    Declaration,
    Assignment,
};

// Only used for printing during testing with gtest
std::ostream& operator<<(std::ostream& os, AstNodeKind kind);

enum class AstLiteralKind {
    Integer,
    Float,
    String,
    Bool,
};

struct AstNodeLiteral;
struct AstNodeIdentifier;
struct AstNodeBinary;
struct AstNodeUnary;
struct AstNodeCall;
struct AstNodeIf;
struct AstNodeFor;
struct AstNodeBreak;
struct AstNodeContinue;
struct AstNodeReturn;
struct AstNodeBlock;
struct AstNodeParameter;
struct AstNodeFunction;
struct AstNodeDeclaration;
struct AstNodeAssignment;

struct AstNode {
    AstNodeKind kind;
    TypeSetHandle* type_set;

    AstNodeLiteral* as_literal();
    AstNodeIdentifier* as_identifier();
    AstNodeBinary* as_binary();
    AstNodeUnary* as_unary();
    AstNodeCall* as_call();
    AstNodeIf* as_if();
    AstNodeFor* as_for();
    AstNodeBreak* as_break();
    AstNodeContinue* as_continue();
    AstNodeReturn* as_return();
    AstNodeBlock* as_block();
    AstNodeParameter* as_parameter();
    AstNodeFunction* as_function();
    AstNodeDeclaration* as_declaration();
    AstNodeAssignment* as_assignment();
};

struct AstNodeLiteral : public AstNode {
    Token token;
    AstLiteralKind literal_kind;

    // Used for compilation
    MemPtr static_data_ptr;

    static AstNodeLiteral* make(Token token, AstLiteralKind kind,
                                Arena* arena) {
        AstNodeLiteral* node = arena_alloc<AstNodeLiteral>(arena);
        node->kind = AstNodeKind::Literal;
        node->token = token;
        node->literal_kind = kind;
        node->static_data_ptr = mem_ptr_invalid();
        return node;
    }
};

struct AstNodeIdentifier : public AstNode {
    Token token;
    AstNode* def;

    // Used for compilation
    MemPtr ptr;

    static AstNodeIdentifier* make(Token token, Arena* arena) {
        AstNodeIdentifier* node = arena_alloc<AstNodeIdentifier>(arena);
        node->kind = AstNodeKind::Identifier;
        node->token = token;
        node->ptr = mem_ptr_invalid();
        return node;
    }
};

struct AstNodeBinary : public AstNode {
    Token token;
    AstNode* left;
    AstNode* right;
    TokenKind op;

    static AstNodeBinary* make(AstNode* left, AstNode* right, Token token,
                               Arena* arena) {
        AstNodeBinary* node = arena_alloc<AstNodeBinary>(arena);
        node->kind = AstNodeKind::Binary;
        node->left = left;
        node->right = right;
        node->token = token;
        node->op = token.kind;
        return node;
    }
};

struct AstNodeUnary : public AstNode {
    Token token;
    AstNode* operand;
    TokenKind op;

    static AstNodeUnary* make(AstNode* operand, Token token, Arena* arena) {
        AstNodeUnary* node = arena_alloc<AstNodeUnary>(arena);
        node->kind = AstNodeKind::Unary;
        node->operand = operand;
        node->token = token;
        node->op = token.kind;
        return node;
    }
};

struct AstNodeCall : public AstNode {
    Token token;
    AstNode* callee;
    Array<AstNode*> arguments;

    static AstNodeCall* make(AstNode* callee, Array<AstNode*> arguments,
                             Token token, Arena* arena) {
        AstNodeCall* node = arena_alloc<AstNodeCall>(arena);
        node->kind = AstNodeKind::Call;
        node->callee = callee;
        node->arguments = arguments;
        node->token = token;
        return node;
    }
};

struct AstNodeIf : public AstNode {
    Token token;
    AstNode* condition;
    AstNode* then_branch;
    AstNode* else_branch;

    static AstNodeIf* make(AstNode* condition, AstNode* then_branch,
                           AstNode* else_branch, Token token, Arena* arena) {
        AstNodeIf* node = arena_alloc<AstNodeIf>(arena);
        node->kind = AstNodeKind::If;
        node->condition = condition;
        node->then_branch = then_branch;
        node->else_branch = else_branch;
        node->token = token;
        return node;
    }
};

// TODO(juraj): public Handle ranged for loop syntax
// for i in 0..10
// for item in array
struct AstNodeFor : public AstNode {
    Token token;
    AstNode* init;
    AstNode* condition;
    AstNode* update;
    AstNodeBlock* then_branch;
    AstNodeBlock* else_branch;

    static AstNodeFor* make(AstNode* init, AstNode* condition, AstNode* update,
                            AstNodeBlock* then_branch,
                            AstNodeBlock* else_branch, Token token,
                            Arena* arena) {
        AstNodeFor* node = arena_alloc<AstNodeFor>(arena);
        node->kind = AstNodeKind::For;
        node->init = init;
        node->condition = condition;
        node->update = update;
        node->then_branch = then_branch;
        node->else_branch = else_branch;
        node->token = token;
        return node;
    }
};

struct AstNodeBreak : public AstNode {
    Token token;
    AstNode* value;

    static AstNodeBreak* make(AstNode* value, Token token, Arena* arena) {
        AstNodeBreak* node = arena_alloc<AstNodeBreak>(arena);
        node->kind = AstNodeKind::Break;
        node->value = value;
        node->token = token;
        return node;
    }
};

struct AstNodeContinue : public AstNode {
    Token token;

    static AstNodeContinue* make(Token token, Arena* arena) {
        AstNodeContinue* node = arena_alloc<AstNodeContinue>(arena);
        node->kind = AstNodeKind::Continue;
        node->token = token;
        return node;
    }
};

struct AstNodeReturn : public AstNode {
    Token token;
    AstNode* value;

    static AstNodeReturn* make(AstNode* value, Token token, Arena* arena) {
        AstNodeReturn* node = arena_alloc<AstNodeReturn>(arena);
        node->kind = AstNodeKind::Return;
        node->value = value;
        node->token = token;
        return node;
    }
};

struct AstNodeBlock : public AstNode {
    Token token;
    Array<AstNode*> statements;

    static AstNodeBlock* make(Array<AstNode*> statements, Token token,
                              Arena* arena) {
        AstNodeBlock* node = arena_alloc<AstNodeBlock>(arena);
        node->kind = AstNodeKind::Block;
        node->statements = statements;
        node->token = token;
        return node;
    }
};

struct AstNodeParameter : public AstNode {
    Token token;
    AstNodeIdentifier* name;
    AstNode* type;

    static AstNodeParameter* make(AstNodeIdentifier* name, AstNode* type,
                                  Token token, Arena* arena) {
        AstNodeParameter* node = arena_alloc<AstNodeParameter>(arena);
        node->kind = AstNodeKind::Parameter;
        node->name = name;
        node->type = type;
        node->token = token;
        return node;
    }
};

struct AstNodeFunction : public AstNode {
    Token token;
    Array<AstNodeParameter*> parameters;
    AstNode* return_type;
    AstNodeBlock* body;

    // Used for compilation
    isize offset;

    static AstNodeFunction* make(Array<AstNodeParameter*> parameters,
                                 AstNode* return_type, AstNodeBlock* body,
                                 Token token, Arena* arena) {
        AstNodeFunction* node = arena_alloc<AstNodeFunction>(arena);
        node->kind = AstNodeKind::Function;
        node->parameters = parameters;
        node->return_type = return_type;
        node->body = body;
        node->offset = -1;
        node->token = token;
        return node;
    }
};

enum class AstDeclarationKind { Variable, Constant };

struct AstNodeDeclaration : public AstNode {
    AstNodeIdentifier* name;
    AstNode* type;
    AstNode* value;
    AstDeclarationKind decl_kind;

    static AstNodeDeclaration* make(AstNodeIdentifier* name, AstNode* type,
                                    AstNode* value, AstDeclarationKind kind,
                                    Arena* arena) {
        AstNodeDeclaration* node = arena_alloc<AstNodeDeclaration>(arena);
        node->kind = AstNodeKind::Declaration;
        node->name = name;
        node->type = type;
        node->value = value;
        node->decl_kind = kind;
        return node;
    }
};

struct AstNodeAssignment : public AstNode {
    Token token;
    AstNode* name;
    AstNode* value;

    static AstNodeAssignment* make(AstNode* name, AstNode* value, Token token,
                                   Arena* arena) {
        AstNodeAssignment* node = arena_alloc<AstNodeAssignment>(arena);
        node->kind = AstNodeKind::Assignment;
        node->name = name;
        node->value = value;
        node->token = token;
        return node;
    }
};

inline AstNodeLiteral* AstNode::as_literal() {
    core_assert(this->kind == AstNodeKind::Literal);
    return static_cast<AstNodeLiteral*>(this);
}

inline AstNodeIdentifier* AstNode::as_identifier() {
    core_assert(this->kind == AstNodeKind::Identifier);
    return static_cast<AstNodeIdentifier*>(this);
}

inline AstNodeBinary* AstNode::as_binary() {
    core_assert(this->kind == AstNodeKind::Binary);
    return static_cast<AstNodeBinary*>(this);
}

inline AstNodeUnary* AstNode::as_unary() {
    core_assert(this->kind == AstNodeKind::Unary);
    return static_cast<AstNodeUnary*>(this);
}

inline AstNodeCall* AstNode::as_call() {
    core_assert(this->kind == AstNodeKind::Call);
    return static_cast<AstNodeCall*>(this);
}

inline AstNodeIf* AstNode::as_if() {
    core_assert(this->kind == AstNodeKind::If);
    return static_cast<AstNodeIf*>(this);
}

inline AstNodeFor* AstNode::as_for() {
    core_assert(this->kind == AstNodeKind::For);
    return static_cast<AstNodeFor*>(this);
}

inline AstNodeBreak* AstNode::as_break() {
    core_assert(this->kind == AstNodeKind::Break);
    return static_cast<AstNodeBreak*>(this);
}

inline AstNodeContinue* AstNode::as_continue() {
    core_assert(this->kind == AstNodeKind::Continue);
    return static_cast<AstNodeContinue*>(this);
}

inline AstNodeReturn* AstNode::as_return() {
    core_assert(this->kind == AstNodeKind::Return);
    return static_cast<AstNodeReturn*>(this);
}

inline AstNodeBlock* AstNode::as_block() {
    core_assert(this->kind == AstNodeKind::Block);
    return static_cast<AstNodeBlock*>(this);
}

inline AstNodeParameter* AstNode::as_parameter() {
    core_assert(this->kind == AstNodeKind::Parameter);
    return static_cast<AstNodeParameter*>(this);
}

inline AstNodeFunction* AstNode::as_function() {
    core_assert(this->kind == AstNodeKind::Function);
    return static_cast<AstNodeFunction*>(this);
}

inline AstNodeDeclaration* AstNode::as_declaration() {
    core_assert(this->kind == AstNodeKind::Declaration);
    return static_cast<AstNodeDeclaration*>(this);
}

inline AstNodeAssignment* AstNode::as_assignment() {
    core_assert(this->kind == AstNodeKind::Assignment);
    return static_cast<AstNodeAssignment*>(this);
}

struct Ast {
    Array<AstNode*> declarations;
};

inline void ast_init(Ast* ast, Arena* arena) {
    array_init(&ast->declarations, 16, arena);
}

String ast_serialize_debug(AstNode* node, Arena* arena);
