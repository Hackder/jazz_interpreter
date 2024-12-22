#include "parser.hpp"
#include "core.hpp"

Token peek_token(AstFile* file, isize index = 1) {
    core_assert(index > 0);

    while (file->tokens.size < index) {
        TokenizerResult token = tokenizer_next_token(&file->tokenizer);
        // TODO(juraj): handle errors
        core_assert(token.error == TokenizerErrorKind::None);
        ring_buffer_push_end(&file->tokens, token.token);
    }

    return file->tokens[index - 1];
}

Token next_token(AstFile* file) {
    Token tok = peek_token(file);
    ring_buffer_pop_front(&file->tokens);
    return tok;
}

AstNode* parse_type(AstFile* file, Arena* arena) {
    Token tok = next_token(file);
    if (tok.kind == TokenKind::Identifier) {
        return AstNodeIdentifier::make(tok, arena);
    }

    // TODO(juraj): support for function type literals
    core_assert(false);
}

bool is_binary_operator(Token tok) {
    switch (tok.kind) {
    case TokenKind::Plus:
    case TokenKind::Minus:
    case TokenKind::Asterisk:
    case TokenKind::Slash:
    case TokenKind::LessThan:
    case TokenKind::LessEqual:
    case TokenKind::GreaterThan:
    case TokenKind::GreaterEqual:
    case TokenKind::Equal:
    case TokenKind::NotEqual:
    case TokenKind::BinaryAnd:
    case TokenKind::BinaryOr:
    case TokenKind::LogicalAnd:
    case TokenKind::LogicalOr:
        return true;
    default:
        return false;
    }
}

isize operator_precedence(Token tok) {
    switch (tok.kind) {
    case TokenKind::Equal:
    case TokenKind::NotEqual:
    case TokenKind::LessThan:
    case TokenKind::LessEqual:
    case TokenKind::GreaterThan:
    case TokenKind::GreaterEqual:
        return 1;
    case TokenKind::BinaryAnd:
        return 2;
    case TokenKind::BinaryOr:
        return 3;
    case TokenKind::LogicalAnd:
        return 4;
    case TokenKind::LogicalOr:
        return 5;
    case TokenKind::Plus:
    case TokenKind::Minus:
        return 6;
    case TokenKind::Asterisk:
    case TokenKind::Slash:
        return 7;
    case TokenKind::Bang:
        return 8;
    default:
        return 0;
    }
}

AstNode* parse_expression(AstFile* file, Arena* arena);

AstNodeBlock* parse_block(AstFile* file, Arena* arena) {
    Token tok = next_token(file);
    core_assert(tok.kind == TokenKind::LBrace);

    Array<AstNode*> statements;
    array_init<AstNode*>(&statements, 16, arena);

    while (true) {
        Token next = peek_token(file);
        if (next.kind == TokenKind::RBrace) {
            break;
        }

        if (next.kind == TokenKind::Eof) {
            core_assert(false);
        }

        AstNode* statement = parse_statement(file, arena);
        array_push<AstNode*>(&statements, statement);
    }

    tok = next_token(file);
    core_assert(tok.kind == TokenKind::RBrace);

    return AstNodeBlock::make(statements, tok, arena);
}

AstNodeFunction* parse_function_expression(AstFile* file, Arena* arena) {
    Token fn_keyword = next_token(file);
    core_assert(fn_keyword.kind == TokenKind::Func);

    Token tok = next_token(file);
    core_assert(tok.kind == TokenKind::LParen);

    Array<AstNodeParameter*> parameters;
    array_init(&parameters, 8, arena);

    while (true) {
        Token next = peek_token(file);
        if (next.kind == TokenKind::RParen) {
            break;
        }

        Token name = next_token(file);
        core_assert(name.kind == TokenKind::Identifier);

        next = peek_token(file);

        AstNode* type = nullptr;
        if (next.kind == TokenKind::Colon) {
            next_token(file);
            type = parse_type(file, arena);
            next = peek_token(file);
        }

        if (next.kind == TokenKind::Comma) {
            next_token(file);
        }

        AstNodeParameter* parameter = AstNodeParameter::make(
            AstNodeIdentifier::make(name, arena), type, next, arena);

        array_push(&parameters, parameter);
    }

    tok = next_token(file);
    core_assert(tok.kind == TokenKind::RParen);

    AstNodeBlock* body = parse_block(file, arena);

    return AstNodeFunction::make(parameters, nullptr, body, fn_keyword, arena);
}

// Parses everything that a binary operator can be applied to.
// This includes literals, identifiers, unary operators, and more complex
// expressions.
AstNode* parse_expression_operand(AstFile* file, Arena* arena) {
    Token tok = peek_token(file);
    switch (tok.kind) {
    case TokenKind::Integer: {
        Token tok = next_token(file);
        return AstNodeLiteral::make(tok, AstLiteralKind::Integer, arena);
    }
    case TokenKind::String: {
        Token tok = next_token(file);
        return AstNodeLiteral::make(tok, AstLiteralKind::String, arena);
    }
    case TokenKind::Identifier: {
        Token tok = next_token(file);
        Token next = peek_token(file);
        if (next.kind == TokenKind::LParen) {
            // TODO: Function call
            core_assert(false);
        } else if (next.kind == TokenKind::LBracket) {
            // TODO: Array access
            core_assert(false);
        } else if (next.kind == TokenKind::Period) {
            // TODO: Struct field access
            core_assert(false);
        }

        return AstNodeIdentifier::make(tok, arena);
    }
    case TokenKind::LParen: {
        Token tok = next_token(file);
        AstNode* inner = parse_expression(file, arena);

        tok = next_token(file);
        core_assert(tok.kind == TokenKind::RParen);
        return inner;
    }
    // Unary operators
    case TokenKind::Plus:
    case TokenKind::Minus:
    case TokenKind::Bang: {
        Token tok = next_token(file);
        AstNode* operand = parse_expression_operand(file, arena);
        return AstNodeUnary::make(operand, tok, arena);
    }
    // More complex expressions
    case TokenKind::If: {
        Token tok = next_token(file);
        AstNode* condition = parse_expression(file, arena);
        AstNode* then_branch = parse_block(file, arena);
    }
    case TokenKind::For:
    case TokenKind::Func: {
        return parse_function_expression(file, arena);
    }
    // Invalid
    default:
        core_assert(false);
    }
}

AstNode* parse_expression_rec(AstFile* file, isize precedence, Arena* arena) {
    AstNode* left = parse_expression_operand(file, arena);

    bool valid = true;
    while (valid) {
        Token tok = peek_token(file);
        if (!is_binary_operator(tok)) {
            break;
        }

        isize next_precedence = operator_precedence(tok);
        if (next_precedence <= precedence) {
            break;
        }

        tok = next_token(file);
        AstNode* right = parse_expression_rec(file, next_precedence, arena);
        AstNode* binary = AstNodeBinary::make(left, right, tok, arena);
        left = binary;
    }

    return left;
}

AstNode* parse_expression(AstFile* file, Arena* arena) {
    return parse_expression_rec(file, 0, arena);
}

AstNode* parse_declaration(AstFile* file, Arena* arena) {
    Token tok = next_token(file);
    core_assert(tok.kind == TokenKind::Identifier);
    AstNodeIdentifier* name = AstNodeIdentifier::make(tok, arena);

    Token colon = next_token(file);
    core_assert(colon.kind == TokenKind::Colon);

    AstNode* type = nullptr;
    tok = next_token(file);
    if (tok.kind != TokenKind::Assign && tok.kind != TokenKind::Colon) {
        type = parse_type(file, arena);
        tok = next_token(file);
    }

    if (tok.kind == TokenKind::Colon) {
        AstNode* value = parse_expression(file, arena);

        return AstNodeDeclaration::make(name, type, value,
                                        AstDeclarationKind::Constant, arena);
    } else if (tok.kind == TokenKind::Assign) {
        AstNode* value = parse_expression(file, arena);

        return AstNodeDeclaration::make(name, type, value,
                                        AstDeclarationKind::Variable, arena);
    }

    core_assert(false);
}

AstNode* parse_statement(AstFile* file, Arena* arena) {
    core_assert(file);
    core_assert(arena);

    Token tok = peek_token(file);
    switch (tok.kind) {
    case TokenKind::Identifier: {
        if (peek_token(file, 2).kind == TokenKind::Colon) {
            return parse_declaration(file, arena);
        }

        if (peek_token(file, 2).kind == TokenKind::Assign) {
            Token iden_tok = next_token(file);
            Token assign_tok = next_token(file);

            AstNode* value = parse_expression(file, arena);
            AstNode* assign = AstNodeAssignment::make(
                AstNodeIdentifier::make(iden_tok, arena), value, assign_tok,
                arena);

            return assign;
        }

        return parse_expression(file, arena);
    }
    case TokenKind::LParen:
        return parse_expression(file, arena);
    case TokenKind::LBrace:
        return parse_block(file, arena);
    case TokenKind::If:
        return nullptr;
    case TokenKind::For:
        return nullptr;
    case TokenKind::Func:
        return nullptr;
    default:
        return parse_expression(file, arena);
    }
}

Ast* ast_file_parse(AstFile* file, Arena* arena) { core_assert(false); }