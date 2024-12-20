#include "ast.hpp"
#include "core.hpp"
#include <sstream>

void ast_serialize_debug_rec(AstNode* node, std::ostream& stream) {
    switch (node->kind) {
    case AstNodeKind::Literal: {
        stream << "Lit(";
        stream << node->literal.token.source;
        stream << ")";
        break;
    }
    case AstNodeKind::Identifier: {
        stream << "Ident(";
        stream << node->identifier.token.source;
        stream << ")";
        break;
    }
    case AstNodeKind::Binary: {
        stream << "Bin(";
        ast_serialize_debug_rec(node->binary_expr.left, stream);
        stream << " ";
        stream << node->binary_expr.token.source;
        stream << " ";
        ast_serialize_debug_rec(node->binary_expr.right, stream);
        stream << ")";
        break;
    }
    case AstNodeKind::Unary: {
        stream << "Unary(";
        stream << node->unary_expr.token.source;
        stream << " ";
        ast_serialize_debug_rec(node->unary_expr.operand, stream);
        stream << ")";
        break;
    }
    case AstNodeKind::Call: {
        stream << "Call(";
        ast_serialize_debug_rec(node->call_expr.callee, stream);
        for (int i = 0; i < node->call_expr.arguments.size; i++) {
            stream << " ";
            ast_serialize_debug_rec(node->call_expr.arguments[i], stream);
        }
        stream << ")";
        break;
    }
    case AstNodeKind::If: {
        stream << "If(";
        ast_serialize_debug_rec(node->if_expr.condition, stream);
        stream << " ";
        ast_serialize_debug_rec(node->if_expr.then_branch, stream);
        if (node->if_expr.else_branch) {
            stream << " ";
            ast_serialize_debug_rec(node->if_expr.else_branch, stream);
        }
        stream << ")";
        break;
    }
    case AstNodeKind::For: {
        stream << "For(";
        ast_serialize_debug_rec(node->for_expr.init, stream);
        stream << " ";
        ast_serialize_debug_rec(node->for_expr.condition, stream);
        stream << " ";
        ast_serialize_debug_rec(node->for_expr.update, stream);
        stream << " ";
        ast_serialize_debug_rec(node->for_expr.body, stream);
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
        if (node->return_expr.value) {
            ast_serialize_debug_rec(node->return_expr.value, stream);
        }
        stream << ")";
        break;
    }
    case AstNodeKind::Block: {
        stream << "Block(";
        for (isize i = 0; i < node->block.statements.size; i++) {
            ast_serialize_debug_rec(node->block.statements[i], stream);
            stream << " ";
        }
        stream << ")";
        break;
    }
    case AstNodeKind::Parameter: {
        stream << "Param(";
        stream << node->parameter.name.token.source;
        stream << ")";
        break;
    }
    case AstNodeKind::Function: {
        stream << "Func(";
        stream << node->function.name.token.source;
        stream << " ";
        for (isize i = 0; i < node->function.parameters.size; i++) {
            AstNode n = {
                .kind = AstNodeKind::Parameter,
                .parameter = node->function.parameters[i],
            };
            ast_serialize_debug_rec(&n, stream);
            stream << " ";
        }
        AstNode n = {
            .kind = AstNodeKind::Block,
            .block = node->function.body,
        };
        ast_serialize_debug_rec(&n, stream);
        stream << ")";
        break;
    }
    case AstNodeKind::Declaration: {
        stream << "Decl(";
        stream << node->declaration.name.token.source;

        switch (node->declaration.kind) {
        case AstDeclarationKind::Constant:
            stream << " :: ";
            break;
        case AstDeclarationKind::Variable:
            stream << " := ";
            break;
        }

        ast_serialize_debug_rec(node->declaration.value, stream);
        stream << ")";
        break;
    }
    case AstNodeKind::Assignment: {
        stream << "Assign(";
        stream << node->assignment.name.token.source;
        stream << " ";
        ast_serialize_debug_rec(node->assignment.value, stream);
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
