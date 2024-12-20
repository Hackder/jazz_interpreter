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
inline std::ostream& operator<<(std::ostream& os, AstNodeKind kind) {
    switch (kind) {
    case AstNodeKind::Literal:
        os << "Literal";
        break;
    case AstNodeKind::Identifier:
        os << "Identifier";
        break;
    case AstNodeKind::Binary:
        os << "Binary";
        break;
    case AstNodeKind::Unary:
        os << "Unary";
        break;
    case AstNodeKind::Call:
        os << "Call";
        break;
    case AstNodeKind::If:
        os << "If";
        break;
    case AstNodeKind::For:
        os << "For";
        break;
    case AstNodeKind::Break:
        os << "Break";
        break;
    case AstNodeKind::Continue:
        os << "Continue";
        break;
    case AstNodeKind::Return:
        os << "Return";
        break;
    case AstNodeKind::Block:
        os << "Block";
        break;
    case AstNodeKind::Parameter:
        os << "Parameter";
        break;
    case AstNodeKind::Function:
        os << "Function";
        break;
    case AstNodeKind::Declaration:
        os << "Declaration";
        break;
    case AstNodeKind::Assignment:
        os << "Assignment";
        break;
    }

    return os;
}

enum class AstLiteralKind {
    Integer,
    Float,
    String,
    Bool,
};

struct AstNode;

struct AstNodeLiteral {
    Token token;
    AstLiteralKind kind;
};

struct AstNodeIdentifier {
    Token token;
};

struct AstNodeBinary {
    Token token;
    AstNode* left;
    AstNode* right;
    TokenKind op;
};

struct AstNodeUnary {
    Token token;
    AstNode* operand;
    TokenKind op;
};

struct AstNodeCall {
    Token token;
    AstNode* callee;
    Array<AstNode*> arguments;
};

struct AstNodeIf {
    Token token;
    AstNode* condition;
    AstNode* then_branch;
    AstNode* else_branch;
};

// TODO(juraj): Handle ranged for loop syntax
// for i in 0..10
// for item in array
struct AstNodeFor {
    Token token;
    AstNode* init;
    AstNode* condition;
    AstNode* update;
    AstNode* body;
    AstNode* else_branch;
};

struct AstNodeBreak {
    Token token;
    AstNode* value;
};

struct AstNodeContinue {
    Token token;
};

struct AstNodeReturn {
    Token token;
    AstNode* value;
};

struct AstNodeBlock {
    Token token;
    Array<AstNode*> statements;
};

struct AstNodeParameter {
    Token token;
    AstNodeIdentifier name;
    AstNode* type;
};

struct AstNodeFunction {
    Token token;
    AstNodeIdentifier name;
    Array<AstNodeParameter> parameters;
    AstNode* return_type;
    AstNodeBlock body;
};

enum class AstDeclarationKind { Variable, Constant };

struct AstNodeDeclaration {
    AstNodeIdentifier name;
    AstNode* type;
    AstNode* value;
    AstDeclarationKind kind;
};

struct AstNodeAssignment {
    Token token;
    AstNodeIdentifier name;
    AstNode* value;
};

struct AstNode {
    AstNodeKind kind;
    union {
        AstNodeLiteral literal;
        AstNodeIdentifier identifier;
        AstNodeBinary binary_expr;
        AstNodeUnary unary_expr;
        AstNodeCall call_expr;
        AstNodeIf if_expr;
        AstNodeFor for_expr;
        AstNodeBreak break_expr;
        AstNodeContinue continue_expr;
        AstNodeReturn return_expr;
        AstNodeBlock block;
        AstNodeParameter parameter;
        AstNodeFunction function;
        AstNodeDeclaration declaration;
        AstNodeAssignment assignment;
    };
};

struct Ast {
    Array<AstNode*> declarations;
};

inline AstNode* ast_literal_make(Token token, AstLiteralKind kind,
                                 Arena* arena) {
    AstNode* node = arena_alloc<AstNode>(arena);
    node->kind = AstNodeKind::Literal;
    node->literal.token = token;
    node->literal.kind = kind;
    return node;
}

inline AstNode* ast_identifier_make(Token token, Arena* arena) {
    AstNode* node = arena_alloc<AstNode>(arena);
    node->kind = AstNodeKind::Identifier;
    node->identifier.token = token;
    return node;
}

inline AstNode* ast_binary_make(AstNode* left, AstNode* right, Token token,
                                Arena* arena) {
    AstNode* node = arena_alloc<AstNode>(arena);
    node->kind = AstNodeKind::Binary;
    node->binary_expr.left = left;
    node->binary_expr.right = right;
    node->binary_expr.token = token;
    return node;
}

inline AstNode* ast_unary_make(AstNode* operand, Token token, Arena* arena) {
    AstNode* node = arena_alloc<AstNode>(arena);
    node->kind = AstNodeKind::Unary;
    node->unary_expr.operand = operand;
    node->unary_expr.token = token;
    return node;
}

inline AstNode* ast_declaration_make(AstNodeIdentifier name, AstNode* type,
                                     AstNode* value, AstDeclarationKind kind,
                                     Arena* arena) {
    AstNode* node = arena_alloc<AstNode>(arena);
    node->kind = AstNodeKind::Declaration;
    node->declaration.name = name;
    node->declaration.type = type;
    node->declaration.value = value;
    node->declaration.kind = kind;
    return node;
}

String ast_serialize_debug(AstNode* node, Arena* arena);
