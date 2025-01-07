#include "sema.hpp"
#include "ast.hpp"
#include "core.hpp"
#include <iostream>

struct SemaContext {
    Array<HashMap<String, AstNodeIdentifier*>> defs;
    bool inside_loop;
};

void sema_context_init(SemaContext* context, Arena* arena) {
    array_init(&context->defs, 5, arena);
}

AstNodeIdentifier* sema_context_define_value(SemaContext* context,
                                             AstNode* ident) {
    core_assert(context->defs.size > 0);
    HashMap<String, AstNodeIdentifier*>* current_context =
        &context->defs[context->defs.size - 1];

    AstNodeIdentifier* def = ident->as_identifier();
    hash_map_insert_or_set(current_context, def->token.source, def);

    return def;
}

AstNodeIdentifier* sema_context_get_def_ptr(SemaContext* context,
                                            AstNode* ident) {
    String name = ident->as_identifier()->token.source;
    for (isize i = context->defs.size - 1; i >= 0; i--) {
        HashMap<String, AstNodeIdentifier*>* current_context =
            &context->defs[i];
        AstNodeIdentifier** id = hash_map_get_ptr(current_context, name);
        if (id != nullptr) {
            return *id;
        }
    }

    return nullptr;
}

void sema_context_push_context(SemaContext* context) {
    HashMap<String, AstNodeIdentifier*> new_context;
    hash_map_init(&new_context, 5, context->defs.arena);
    array_push(&context->defs, new_context);
}

void sema_context_pop_context(SemaContext* context) {
    core_assert(context->defs.size > 0);
    context->defs.size -= 1;
}

void analyse_top_level_declarations(AstFile* file, SemaContext* context) {
    for (isize i = 0; i < file->ast.declarations.size; i++) {
        AstNodeDeclaration* node = file->ast.declarations[i]->as_declaration();

        AstNodeIdentifier* id = sema_context_get_def_ptr(context, node->name);
        // TODO(juraj): report errors to the user
        core_assert(id == nullptr);

        sema_context_define_value(context, node->name);
    }
}

void analyse_expression(AstFile* file, SemaContext* context, AstNode* node,
                        Arena* arena);
void analyse_statement(AstFile* file, SemaContext* context, AstNode* node,
                       Arena* arena);

void analyse_block(AstFile* file, SemaContext* context, AstNode* node,
                   Arena* arena) {
    AstNodeBlock* block = node->as_block();

    sema_context_push_context(context);
    for (isize i = 0; i < block->statements.size; i++) {
        analyse_statement(file, context, block->statements[i], arena);
    }
    sema_context_pop_context(context);
}

void analyse_expression(AstFile* file, SemaContext* context, AstNode* node,
                        Arena* arena) {
    switch (node->kind) {
    case AstNodeKind::Literal: {
        AstNodeLiteral* literal = node->as_literal();
        switch (literal->literal_kind) {
        case AstLiteralKind::Integer:
            literal->type = Type::get_int();
            break;
        case AstLiteralKind::Float:
            literal->type = Type::get_float();
            break;
        case AstLiteralKind::String:
            literal->type = Type::get_string();
            break;
        case AstLiteralKind::Bool:
            literal->type = Type::get_bool();
            break;
        }
        break;
    }
    case AstNodeKind::Identifier: {
        AstNodeIdentifier* definition = sema_context_get_def_ptr(context, node);
        if (definition == nullptr) {
            // TODO(juraj): report error to the user
            core_assert_msg(false, "Undefined identifier: %.*s",
                            node->as_identifier()->token.source.size,
                            node->as_identifier()->token.source.data);
        }

        // node->type = definition->type;
        // core_assert(node->type);
        // node->as_identifier()->def = definition;
        break;
    }
    case AstNodeKind::Binary: {
        AstNodeBinary* bin = node->as_binary();
        analyse_expression(file, context, bin->left, arena);
        analyse_expression(file, context, bin->right, arena);

        // switch (bin->op) {
        // case TokenKind::Plus: {
        //     core_assert(bin->left->type == bin->right->type);
        //     core_assert(bin->left->type->kind == TypeKind::Integer ||
        //                 bin->left->type->kind == TypeKind::Float ||
        //                 bin->left->type->kind == TypeKind::String);
        //     break;
        // }
        //
        // case TokenKind::Minus:
        // case TokenKind::Asterisk:
        // case TokenKind::Slash:
        // case TokenKind::LessThan:
        // case TokenKind::LessEqual:
        // case TokenKind::GreaterThan:
        // case TokenKind::GreaterEqual: {
        //     core_assert(bin->left->type == bin->right->type);
        //     core_assert(bin->left->type->kind == TypeKind::Integer ||
        //                 bin->left->type->kind == TypeKind::Float);
        //     break;
        // }
        // case TokenKind::Assign:
        // case TokenKind::Equal:
        // case TokenKind::NotEqual: {
        //     core_assert(bin->left->type == bin->right->type);
        //     break;
        // }
        // case TokenKind::BinaryAnd:
        // case TokenKind::BinaryOr: {
        //     core_assert(bin->left->type == bin->right->type);
        //     core_assert(bin->left->type->kind == TypeKind::Integer);
        //     break;
        // }
        // case TokenKind::LogicalAnd:
        // case TokenKind::LogicalOr: {
        //     core_assert(bin->left->type == bin->right->type);
        //     core_assert(bin->left->type->kind == TypeKind::Bool);
        //     break;
        // }
        // case TokenKind::LBracket: {
        //     // Array access
        //     core_assert_msg(false, "not implemented");
        //     break;
        // }
        // case TokenKind::Period: {
        //     // Struct member access
        //     core_assert_msg(false, "not implemented");
        //     break;
        // }
        // default:
        //     core_assert_msg(false, "Unexpected binary operator");
        //     break;
        // }
        //
        // node->type = bin->left->type;
        break;
    }
    case AstNodeKind::Unary: {
        AstNodeUnary* unary = node->as_unary();
        analyse_expression(file, context, unary->operand, arena);
        // switch (unary->op) {
        // case TokenKind::Plus:
        // case TokenKind::Minus: {
        //     core_assert(unary->operand->type->kind == TypeKind::Integer ||
        //                 unary->operand->type->kind == TypeKind::Float);
        //     break;
        // }
        // case TokenKind::Bang: {
        //     core_assert(unary->operand->type->kind == TypeKind::Bool);
        //     break;
        // }
        // default:
        //     core_assert_msg(false, "Unexpected unary operator");
        //     break;
        // }
        // node->type = unary->operand->type;
        break;
    }
    case AstNodeKind::Call: {
        AstNodeCall* call = node->as_call();
        analyse_expression(file, context, call->callee, arena);
        for (isize i = 0; i < call->arguments.size; i++) {
            analyse_expression(file, context, call->arguments[i], arena);
        }

        // core_assert(call->callee->type->kind == TypeKind::Function);
        // FunctionType* callee_type = call->callee->type->as_function();
        // core_assert(call->arguments.size == callee_type->parameters.size);
        //
        // for (isize i = 0; i < call->arguments.size; i++) {
        //     core_assert(call->arguments[i]->type ==
        //     callee_type->parameters[i]);
        // }
        //
        // node->type = callee_type->return_type;
        break;
    }
    case AstNodeKind::If: {
        AstNodeIf* if_node = node->as_if();
        analyse_expression(file, context, if_node->condition, arena);

        core_assert(if_node->then_branch);

        analyse_block(file, context, if_node->then_branch, arena);

        // Else branch must be present if the `if` is used as an expression
        core_assert(if_node->else_branch);

        analyse_block(file, context, if_node->else_branch, arena);
        break;
    }
    case AstNodeKind::For: {
        AstNodeFor* for_node = node->as_for();

        bool is_while = for_node->init == nullptr &&
                        for_node->condition != nullptr &&
                        for_node->update == nullptr;

        bool is_for = for_node->init != nullptr &&
                      for_node->condition != nullptr &&
                      for_node->update != nullptr;

        bool is_infinite = for_node->init == nullptr &&
                           for_node->condition == nullptr &&
                           for_node->update == nullptr;

        core_assert(is_while || is_for || is_infinite);

        sema_context_push_context(context);

        if (for_node->init) {
            analyse_statement(file, context, for_node->init, arena);
        }

        if (for_node->condition) {
            analyse_expression(file, context, for_node->condition, arena);

            // TODO(juraj): expression must evaluate to a boolean
        }

        if (for_node->update) {
            analyse_statement(file, context, for_node->update, arena);
        }

        core_assert(for_node->then_branch);
        context->inside_loop = true;
        analyse_block(file, context, for_node->then_branch, arena);
        context->inside_loop = false;
        sema_context_pop_context(context);

        if (is_infinite) {
            core_assert(for_node->else_branch == nullptr);
        } else {
            core_assert(for_node->else_branch);
            analyse_block(file, context, for_node->else_branch, arena);
        }

        break;
    }
    case AstNodeKind::Function: {
        AstNodeFunction* func = node->as_function();
        sema_context_push_context(context);
        for (isize i = 0; i < func->parameters.size; i++) {
            sema_context_define_value(context, func->parameters[i]->name);
        }

        analyse_block(file, context, func->body, arena);
        sema_context_pop_context(context);
        break;
    }
    case AstNodeKind::Block:
    case AstNodeKind::Break:
    case AstNodeKind::Continue:
    case AstNodeKind::Return:
    case AstNodeKind::Parameter:
    case AstNodeKind::Declaration:
    case AstNodeKind::Assignment: {
        std::cerr << node->kind << std::endl;
        core_assert_msg(false, "Unexpected node, expected expression");
    }
    }
}

void analyse_statement(AstFile* file, SemaContext* context, AstNode* node,
                       Arena* arena) {
    switch (node->kind) {
    case AstNodeKind::Block: {
        analyse_block(file, context, node, arena);
        return;
    }
    case AstNodeKind::Break: {
        AstNodeBreak* break_node = node->as_break();
        core_assert(context->inside_loop);
        if (break_node->value) {
            analyse_expression(file, context, break_node->value, arena);
        }
        return;
    }
    case AstNodeKind::Continue: {
        core_assert(context->inside_loop);
        return;
    }
    case AstNodeKind::Return: {
        AstNodeReturn* return_node = node->as_return();
        if (return_node->value) {
            analyse_expression(file, context, return_node->value, arena);
        }
        return;
    }
    case AstNodeKind::Declaration: {
        AstNodeDeclaration* decl = node->as_declaration();
        if (decl->value) {
            analyse_expression(file, context, decl->value, arena);
        }
        sema_context_define_value(context, decl->name);
        return;
    }
    case AstNodeKind::Assignment: {
        AstNodeAssignment* assign = node->as_assignment();
        analyse_expression(file, context, assign->name, arena);
        analyse_expression(file, context, assign->value, arena);
        return;
    }
    case AstNodeKind::If: {
        AstNodeIf* if_node = node->as_if();
        analyse_expression(file, context, if_node->condition, arena);

        core_assert(if_node->then_branch);

        analyse_block(file, context, if_node->then_branch, arena);

        if (if_node->else_branch) {
            analyse_block(file, context, if_node->else_branch, arena);
        }

        return;
    }
    case AstNodeKind::For: {
        AstNodeFor* for_node = node->as_for();

        bool is_while = for_node->init == nullptr &&
                        for_node->condition != nullptr &&
                        for_node->update == nullptr;

        bool is_for = for_node->init != nullptr &&
                      for_node->condition != nullptr &&
                      for_node->update != nullptr;

        bool is_infinite = for_node->init == nullptr &&
                           for_node->condition == nullptr &&
                           for_node->update == nullptr;

        core_assert(is_while || is_for || is_infinite);

        sema_context_push_context(context);

        if (for_node->init) {
            analyse_statement(file, context, for_node->init, arena);
        }

        if (for_node->condition) {
            analyse_expression(file, context, for_node->condition, arena);

            // Expression must evaluate to a boolean
        }

        if (for_node->update) {
            analyse_statement(file, context, for_node->update, arena);
        }

        core_assert(for_node->then_branch);
        context->inside_loop = true;
        analyse_block(file, context, for_node->then_branch, arena);
        context->inside_loop = false;
        sema_context_pop_context(context);

        if (for_node->else_branch) {
            if (is_infinite) {
                core_assert_msg(false,
                                "Infinite loop must not have an else branch");
            }

            analyse_block(file, context, for_node->else_branch, arena);
        }

        return;
    }
    case AstNodeKind::Parameter:
        core_assert_msg(false, "Unexpected node, expected statement");

    default:
        break;
    }

    analyse_expression(file, context, node, arena);
}

void semantic_analysis(AstFile* file, Arena* arena) {
    Arena sema_arena;
    arena_init(&sema_arena, 16 * 1024);
    defer(arena_free(&sema_arena));

    SemaContext context;
    sema_context_init(&context, &sema_arena);

    sema_context_push_context(&context);
    analyse_top_level_declarations(file, &context);

    for (isize i = 0; i < file->ast.declarations.size; i++) {
        AstNodeDeclaration* node = file->ast.declarations[i]->as_declaration();
        analyse_expression(file, &context, node->value, arena);
    }
}
