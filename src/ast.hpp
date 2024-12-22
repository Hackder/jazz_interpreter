#include "core.hpp"
#include "tokenizer.hpp"

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

    static AstNodeLiteral* make(Token token, AstLiteralKind kind,
                                Arena* arena) {
        AstNodeLiteral* node = arena_alloc<AstNodeLiteral>(arena);
        node->kind = AstNodeKind::Literal;
        node->token = token;
        node->literal_kind = kind;
        return node;
    }
};

struct AstNodeIdentifier : public AstNode {
    Token token;

    static AstNodeIdentifier* make(Token token, Arena* arena) {
        AstNodeIdentifier* node = arena_alloc<AstNodeIdentifier>(arena);
        node->kind = AstNodeKind::Identifier;
        node->token = token;
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

    static AstNodeFunction* make(Array<AstNodeParameter*> parameters,
                                 AstNode* return_type, AstNodeBlock* body,
                                 Token token, Arena* arena) {
        AstNodeFunction* node = arena_alloc<AstNodeFunction>(arena);
        node->kind = AstNodeKind::Function;
        node->parameters = parameters;
        node->return_type = return_type;
        node->body = body;
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

String ast_serialize_debug(AstNode* node, Arena* arena);
