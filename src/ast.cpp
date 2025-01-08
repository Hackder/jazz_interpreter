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
    if (node == nullptr) {
        return;
    }

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
        stream << " then ";
        ast_serialize_debug_rec(node->as_if()->then_branch, stream);
        if (node->as_if()->else_branch) {
            stream << " else ";
            ast_serialize_debug_rec(node->as_if()->else_branch, stream);
        }
        stream << ")";
        break;
    }
    case AstNodeKind::For: {
        AstNodeFor* for_node = node->as_for();
        stream << "For(";
        if (for_node->init) {
            ast_serialize_debug_rec(for_node->init, stream);
            stream << " ";
        }
        if (for_node->condition) {
            ast_serialize_debug_rec(for_node->condition, stream);
            stream << " ";
        }
        if (for_node->update) {
            ast_serialize_debug_rec(for_node->update, stream);
            stream << " ";
        }
        stream << "then ";
        ast_serialize_debug_rec(for_node->then_branch, stream);
        if (for_node->else_branch) {
            stream << " else ";
            ast_serialize_debug_rec(for_node->else_branch, stream);
        }
        stream << ")";
        break;
    }
    case AstNodeKind::Break: {
        stream << "Break(";
        if (node->as_break()->value) {
            ast_serialize_debug_rec(node->as_break()->value, stream);
        }
        stream << ")";
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
        if (node->as_function()->return_type) {
            stream << "-> ";
            ast_serialize_debug_rec(node->as_function()->return_type, stream);
            stream << ", ";
        }
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

        stream << " :";
        if (node->as_declaration()->type) {
            ast_serialize_debug_rec(node->as_declaration()->type, stream);
            stream << " ";
        }

        switch (node->as_declaration()->decl_kind) {
        case AstDeclarationKind::Constant:
            stream << ": ";
            break;
        case AstDeclarationKind::Variable:
            stream << "= ";
            break;
        }

        ast_serialize_debug_rec(node->as_declaration()->value, stream);
        stream << ")";
        break;
    }
    case AstNodeKind::Assignment: {
        stream << "Assign(";
        ast_serialize_debug_rec(node->as_assignment()->name, stream);
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

FunctionType* type_set_get_function(TypeSetHandle* handle) {
    core_assert(handle);
    core_assert(handle->set->types.size == 1);
    core_assert(handle->set->types[0]->kind == TypeKind::Function);
    return handle->set->types[0]->as_function();
}

// Takes all backreferences from the second set and assigns them to the first
// set. Basically everyone that referenced the second set now references the
// first set.
void type_set_reassign_all(TypeSetHandle* handle, TypeSetHandle* other) {
    core_assert(handle);
    core_assert(other);

    for (isize i = 0; i < other->backreferences.size; i++) {
        (*other->backreferences[i]) = handle;
        array_push(&handle->backreferences, other->backreferences[i]);
    }
}

// Performs an intersection between the two sets, modifying the first set.
// If the resulting set would be empty, the function returns false. and
// the original set is restored.
// If the resulting set has some elements, the function returns true and leaves
// the set modified.
// The second set is never modified.
bool type_set_intersect_if_result(TypeSetHandle* handle, TypeSetHandle* other) {
    core_assert(handle);
    core_assert(other);

    if (other->set->is_full) {
        type_set_reassign_all(handle, other);
        return true;
    }

    if (handle->set->is_full) {
        type_set_reassign_all(other, handle);
        return true;
    }

    isize original_size = handle->set->types.size;

    for (isize i = 0; i < handle->set->types.size; i++) {
        Type* type = handle->set->types[i];

        bool found = false;
        isize j = 0;
        for (; j < other->set->types.size; j++) {
            if (type->kind == other->set->types[j]->kind) {
                found = true;
                break;
            }
        }
        if (!found) {
            array_remove_at_unstable(&handle->set->types, i);
            i--;
            continue;
        }

        if (type->kind == TypeKind::Function) {
            FunctionType* ftype = type->as_function();
            FunctionType* other_ftype = other->set->types[j]->as_function();

            bool result = function_type_intersect_with(ftype, other_ftype);
            if (!result) {
                array_remove_at_unstable(&handle->set->types, i);
                i--;
                continue;
            }
        }
    }

    if (handle->set->types.size == 0) {
        // HACK(juraj): to allow us to report errors in a reasonable way,
        // we need to know the original sets. This is a hack to allow us to
        // restore the original set.
        handle->set->types.size = original_size;
        return false;
    }

    type_set_reassign_all(handle, other);
    return true;
}

bool function_type_intersect_with(FunctionType* a, FunctionType* b) {
    if (a->parameters.size != b->parameters.size) {
        return false;
    }

    for (isize i = 0; i < a->parameters.size; i++) {
        if (!type_set_intersect_if_result(a->parameters[i], b->parameters[i])) {
            return false;
        }
    }

    return type_set_intersect_if_result(a->return_type, b->return_type);
}
