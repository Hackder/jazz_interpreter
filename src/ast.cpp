#include "ast.hpp"
#include "core.hpp"
#include <sstream>

std::ostream& operator<<(std::ostream& os, AstNodeKind kind) {
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

void ast_serialize_debug_rec(AstNode* node, std::ostream& stream) {
    switch (node->kind) {
    case AstNodeKind::Literal: {
        stream << "Lit(";
        stream << node->as_literal()->token.source;
        stream << ")";
        break;
    }
    case AstNodeKind::Identifier: {
        stream << "Ident(";
        stream << node->as_identifier()->token.source;
        stream << ")";
        break;
    }
    case AstNodeKind::Binary: {
        stream << "Bin(";
        ast_serialize_debug_rec(node->as_binary()->left, stream);
        stream << " ";
        stream << node->as_binary()->token.source;
        stream << " ";
        ast_serialize_debug_rec(node->as_binary()->right, stream);
        stream << ")";
        break;
    }
    case AstNodeKind::Unary: {
        stream << "Unary(";
        stream << node->as_unary()->token.source;
        stream << " ";
        ast_serialize_debug_rec(node->as_unary()->operand, stream);
        stream << ")";
        break;
    }
    case AstNodeKind::Call: {
        stream << "Call(";
        ast_serialize_debug_rec(node->as_call()->callee, stream);
        for (int i = 0; i < node->as_call()->arguments.size; i++) {
            stream << " ";
            ast_serialize_debug_rec(node->as_call()->arguments[i], stream);
        }
        stream << ")";
        break;
    }
    case AstNodeKind::If: {
        stream << "If(";
        ast_serialize_debug_rec(node->as_if()->condition, stream);
        stream << " ";
        ast_serialize_debug_rec(node->as_if()->then_branch, stream);
        if (node->as_if()->else_branch) {
            stream << " ";
            ast_serialize_debug_rec(node->as_if()->else_branch, stream);
        }
        stream << ")";
        break;
    }
    case AstNodeKind::For: {
        stream << "For(";
        ast_serialize_debug_rec(node->as_for()->init, stream);
        stream << " ";
        ast_serialize_debug_rec(node->as_for()->condition, stream);
        stream << " ";
        ast_serialize_debug_rec(node->as_for()->update, stream);
        stream << " ";
        ast_serialize_debug_rec(node->as_for()->then_branch, stream);
        stream << ")";
        break;
    }
    case AstNodeKind::Break: {
        stream << "Break";
        break;
    }
    case AstNodeKind::Continue: {
        stream << "Continue";
        break;
    }
    case AstNodeKind::Return: {
        stream << "Return(";
        if (node->as_return()->value) {
            ast_serialize_debug_rec(node->as_return()->value, stream);
        }
        stream << ")";
        break;
    }
    case AstNodeKind::Block: {
        stream << "Block(";
        for (isize i = 0; i < node->as_block()->statements.size; i++) {
            if (i > 0) {
                stream << " ";
            }
            ast_serialize_debug_rec(node->as_block()->statements[i], stream);
        }
        stream << ")";
        break;
    }
    case AstNodeKind::Parameter: {
        stream << "Param(";
        stream << node->as_parameter()->name->token.source;
        stream << ")";
        break;
    }
    case AstNodeKind::Function: {
        stream << "Func(";
        stream << node->as_function()->token.source;
        stream << " ";
        for (isize i = 0; i < node->as_function()->parameters.size; i++) {
            ast_serialize_debug_rec(node->as_function()->parameters[i], stream);
            stream << " ";
        }
        ast_serialize_debug_rec(node->as_function()->body, stream);
        stream << ")";
        break;
    }
    case AstNodeKind::Declaration: {
        stream << "Decl(";
        stream << node->as_declaration()->name->token.source;

        switch (node->as_declaration()->decl_kind) {
        case AstDeclarationKind::Constant:
            stream << " :: ";
            break;
        case AstDeclarationKind::Variable:
            stream << " := ";
            break;
        }

        ast_serialize_debug_rec(node->as_declaration()->value, stream);
        stream << ")";
        break;
    }
    case AstNodeKind::Assignment: {
        stream << "Assign(";
        stream << node->as_assignment()->name->token.source;
        stream << " ";
        ast_serialize_debug_rec(node->as_assignment()->value, stream);
        stream << ")";
        break;
    }
    }
}

String ast_serialize_debug(AstNode* node, Arena* arena) {
    std::stringstream stream;
    ast_serialize_debug_rec(node, stream);

    char* buffer = arena_alloc<char>(arena, stream.str().size() + 1);
    memcpy(buffer, stream.str().c_str(), stream.str().size());
    return string_from_cstr(buffer);
}
