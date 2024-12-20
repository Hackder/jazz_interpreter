#include "parser.hpp"
#include "core.hpp"
#include <iostream>

Token next_token(AstFile* file) {
    if (file->peeked_token.kind != TokenKind::Invalid) {
        Token tok = file->peeked_token;
        file->peeked_token = Token{TokenKind::Invalid, {}};
        return tok;
    }

    TokenizerResult token = tokenizer_next_token(&file->tokenizer);
    // TODO(juraj): handle errors
    assert(token.error == TokenizerErrorKind::None);
    return token.token;
}

Token peek_next_token(AstFile* file) {
    if (file->peeked_token.kind == TokenKind::Invalid) {
        TokenizerResult token = tokenizer_next_token(&file->tokenizer);
        // TODO(juraj): handle errors
        assert(token.error == TokenizerErrorKind::None);
        file->peeked_token = token.token;
    }

    return file->peeked_token;
}

AstNode* parse_type(AstFile* file, Arena* arena) {
    Token tok = next_token(file);
    if (tok.kind == TokenKind::Identifier) {
        return ast_identifier_make(tok, arena);
    }

    // TODO(juraj): support for function type literals
    assert(false);
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

AstNode* parse_expression_value(AstFile* file, Arena* arena) {
    Token tok = peek_next_token(file);
    switch (tok.kind) {
    case TokenKind::Integer: {
        Token tok = next_token(file);
        return ast_literal_make(tok, AstLiteralKind::Integer, arena);
    }
    case TokenKind::String: {
        Token tok = next_token(file);
        return ast_literal_make(tok, AstLiteralKind::String, arena);
    }
    case TokenKind::Identifier: {
        Token tok = next_token(file);
        Token next = peek_next_token(file);
        if (next.kind == TokenKind::LParen) {
            // TODO: Function call
            assert(false);
        } else if (next.kind == TokenKind::LBracket) {
            // TODO: Array access
            assert(false);
        } else if (next.kind == TokenKind::Period) {
            // TODO: Struct field access
            assert(false);
        }

        return ast_identifier_make(tok, arena);
    }
    case TokenKind::LParen: {
        Token tok = next_token(file);
        AstNode* inner = parse_expression(file, arena);

        tok = next_token(file);
        assert(tok.kind == TokenKind::RParen);
        return inner;
    }
    // Unary operators
    case TokenKind::Plus:
    case TokenKind::Minus:
    case TokenKind::Bang: {
        Token tok = next_token(file);
        AstNode* operand = parse_expression_value(file, arena);
        return ast_unary_make(operand, tok, arena);
    }
    // More complex expressions
    case TokenKind::If:
    case TokenKind::For:
    case TokenKind::Func: {
        assert(false);
    }
    // Invalid
    default:
        assert(false);
    }
}

AstNode* parse_expression_rec(AstFile* file, isize precedence, Arena* arena) {
    AstNode* left = parse_expression_value(file, arena);

    bool valid = true;
    while (valid) {
        Token tok = peek_next_token(file);
        if (!is_binary_operator(tok)) {
            break;
        }

        isize next_precedence = operator_precedence(tok);
        if (next_precedence <= precedence) {
            break;
        }

        tok = next_token(file);
        AstNode* right = parse_expression_rec(file, next_precedence, arena);
        AstNode* binary = ast_binary_make(left, right, tok, arena);
        left = binary;
    }

    return left;
}

AstNode* parse_expression(AstFile* file, Arena* arena) {
    return parse_expression_rec(file, 0, arena);
}

AstNode* parse_declaration(AstFile* file, Arena* arena) {
    Token tok = next_token(file);
    assert(tok.kind == TokenKind::Identifier);
    AstNodeIdentifier name = {tok};

    Token colon = next_token(file);
    assert(colon.kind == TokenKind::Colon);

    AstNode* type = nullptr;
    tok = next_token(file);
    if (tok.kind != TokenKind::Assign && tok.kind != TokenKind::Colon) {
        type = parse_type(file, arena);
        tok = next_token(file);
    }

    if (tok.kind == TokenKind::Colon) {
        AstNode* value = parse_expression(file, arena);

        return ast_declaration_make(name, type, value,
                                    AstDeclarationKind::Constant, arena);
    } else if (tok.kind == TokenKind::Assign) {
        AstNode* value = parse_expression(file, arena);

        return ast_declaration_make(name, type, value,
                                    AstDeclarationKind::Variable, arena);
    }

    assert(false);
}

Ast* ast_file_parse(AstFile* file, Arena* arena) { assert(false); }
