#include "parser.hpp"
#include "core.hpp"
#include <cstdio>
#include <iostream>

#define check_condition(cond, token, message)                                  \
    if (!(cond)) {                                                             \
        report_error(file, token, message);                                    \
        return nullptr;                                                        \
    }

void report_error(AstFile* file, Token token, const char* message) {
    ParseError error = {
        .token = token,
        .message = string_from_cstr(message),
    };

    array_push(&file->errors, error);
}

Token peek_token(AstFile* file, isize index = 1) {
    core_assert(index > 0);

    while (file->tokens.size < index) {
        TokenizerResult token = tokenizer_next_token(&file->tokenizer);

        switch (token.error) {
        case TokenizerErrorKind::None: {
            ring_buffer_push_end(&file->tokens, token.token);
            break;
        }
        case TokenizerErrorKind::UnclosedString: {
            report_error(file, token.token, "Unclosed string  literal");
            ring_buffer_push_end(
                &file->tokens, Token{TokenKind::Invalid, string_from_cstr("")});
            break;
        }
        case TokenizerErrorKind::InvalidCharacter: {
            report_error(file, token.token, "Invalid character");
            ring_buffer_push_end(
                &file->tokens, Token{TokenKind::Invalid, string_from_cstr("")});
            break;
        }
        }
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
    // fn(int, int) -> int
    report_error(file, tok, "Expected a type");
    return nullptr;
}

bool ast_file_exhausted(AstFile* file) {
    core_assert(file);
    int i = 1;
    while (peek_token(file, i).kind == TokenKind::Newline) {
        i++;
    }

    return peek_token(file, i).kind == TokenKind::Eof;
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
    case TokenKind::Period:
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
    case TokenKind::Period:
    case TokenKind::LParen:
    case TokenKind::LBracket:
        return 9;
    default:
        return 0;
    }
}

AstNode* parse_expression(AstFile* file, bool allow_newlines, Arena* arena);

void skip_newlines(AstFile* file) {
    Token tok = peek_token(file);
    while (tok.kind == TokenKind::Newline) {
        next_token(file);
        tok = peek_token(file);
    }
}

AstNodeBlock* parse_block(AstFile* file, Arena* arena) {
    Token tok = next_token(file);
    check_condition(tok.kind == TokenKind::LBrace, tok, "Expected '{'");

    Array<AstNode*> statements;
    array_init<AstNode*>(&statements, 16, arena);

    while (true) {
        skip_newlines(file);
        Token next = peek_token(file);
        if (next.kind == TokenKind::RBrace) {
            break;
        }

        check_condition(next.kind != TokenKind::Eof, next,
                        "Unexpected end of file, block not closed properly");

        AstNode* statement = parse_statement(file, arena);
        array_push<AstNode*>(&statements, statement);
    }

    tok = next_token(file);
    core_assert(tok.kind == TokenKind::RBrace);

    return AstNodeBlock::make(statements, tok, arena);
}

AstNodeFunction* parse_function_expression(AstFile* file, Arena* arena) {
    Token fn_keyword = next_token(file);
    check_condition(fn_keyword.kind == TokenKind::Func, fn_keyword,
                    "Expected 'fn' keyword (a function definition)");

    Token tok = next_token(file);
    check_condition(tok.kind == TokenKind::LParen, tok, "Expected '('");

    Array<AstNodeParameter*> parameters;
    array_init(&parameters, 8, arena);

    while (true) {
        Token next = peek_token(file);
        if (next.kind == TokenKind::RParen) {
            break;
        }

        Token name = next_token(file);
        check_condition(name.kind == TokenKind::Identifier, name,
                        "Expacted a valid identifier")

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

    AstNode* return_type = nullptr;
    Token next = peek_token(file);
    if (next.kind == TokenKind::Arrow) {
        next_token(file);
        return_type = parse_type(file, arena);
    }

    AstNodeBlock* body = parse_block(file, arena);

    return AstNodeFunction::make(parameters, return_type, body, fn_keyword,
                                 arena);
}

// Parses everything that a binary operator can be applied to.
// This includes literals, identifiers, unary operators, and more complex
// expressions.
AstNode* parse_expression_operand(AstFile* file, bool allow_newlines,
                                  Arena* arena) {
    if (allow_newlines) {
        skip_newlines(file);
    }

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
    case TokenKind::Bool: {
        Token tok = next_token(file);
        return AstNodeLiteral::make(tok, AstLiteralKind::Bool, arena);
    }
    case TokenKind::Identifier: {
        Token tok = next_token(file);
        return AstNodeIdentifier::make(tok, arena);
    }
    case TokenKind::LParen: {
        Token tok = next_token(file);
        AstNode* inner = parse_expression(file, true, arena);

        tok = next_token(file);
        check_condition(tok.kind == TokenKind::RParen, tok, "Expected ')'");
        return inner;
    }
    // Unary operators
    case TokenKind::Plus:
    case TokenKind::Minus:
    case TokenKind::Bang: {
        Token tok = next_token(file);
        AstNode* operand =
            parse_expression_operand(file, allow_newlines, arena);
        return AstNodeUnary::make(operand, tok, arena);
    }
    // More complex expressions
    case TokenKind::If: {
        Token tok = next_token(file);
        AstNode* condition = parse_expression(file, false, arena);
        AstNode* then_branch = parse_block(file, arena);

        AstNode* else_branch = nullptr;
        Token next = peek_token(file);
        if (next.kind == TokenKind::Else) {
            next_token(file);
            else_branch = parse_block(file, arena);
        }

        return AstNodeIf::make(condition, then_branch, else_branch, tok, arena);
    }
    case TokenKind::For: {
        Token tok = next_token(file);
        Token next = peek_token(file);
        AstNode* init = nullptr;
        AstNode* condition = nullptr;
        AstNode* update = nullptr;

        bool infinite_loop = next.kind == TokenKind::LBrace;
        if (!infinite_loop) {
            isize number_of_semicolons = 0;
            for (isize i = 1; true; i++) {
                Token next = peek_token(file, i);
                if (next.kind == TokenKind::Semicolon) {
                    number_of_semicolons++;
                }

                if (next.kind == TokenKind::Newline) {
                    break;
                }

                if (next.kind == TokenKind::LBrace) {
                    break;
                }

                check_condition(
                    next.kind != TokenKind::Eof, next,
                    "Unexpected end of file within 'for' definition");
            }

            if (number_of_semicolons == 2) {
                init = parse_statement(file, arena);
                Token tok = next_token(file);
                check_condition(
                    tok.kind == TokenKind::Semicolon, tok,
                    "For definitions must be separated by semicolons");
                condition = parse_expression(file, false, arena);
                tok = next_token(file);
                check_condition(
                    tok.kind == TokenKind::Semicolon, tok,
                    "For definitions must be separated by semicolons");
                update = parse_statement(file, arena);
            }

            if (number_of_semicolons == 0) {
                condition = parse_expression(file, false, arena);
            }
        }

        AstNodeBlock* then_branch = parse_block(file, arena);

        AstNodeBlock* else_branch = nullptr;
        next = peek_token(file);
        if (next.kind == TokenKind::Else) {
            next_token(file);
            else_branch = parse_block(file, arena);
        }

        return AstNodeFor::make(init, condition, update, then_branch,
                                else_branch, tok, arena);
    }
    case TokenKind::Func: {
        return parse_function_expression(file, arena);
    }
    // Invalid
    default: {
        check_condition(false, tok, "Unexpected token within expression");
    }
    }
}

Array<AstNode*> parse_function_arguments(AstFile* file, Arena* arena) {
    Array<AstNode*> arguments;
    array_init<AstNode*>(&arguments, 8, arena);

    while (true) {
        Token tok = peek_token(file);
        if (tok.kind == TokenKind::RParen) {
            break;
        }

        AstNode* argument = parse_expression(file, true, arena);
        array_push<AstNode*>(&arguments, argument);

        tok = peek_token(file);
        if (tok.kind == TokenKind::Comma) {
            next_token(file);
        }
    }

    return arguments;
}

AstNode* parse_expression_rec(AstFile* file, isize precedence,
                              bool allow_newlines, Arena* arena) {
    AstNode* left = parse_expression_operand(file, allow_newlines, arena);

    while (true) {
        if (allow_newlines) {
            skip_newlines(file);
        }
        Token tok = peek_token(file);

        // Function call
        if (tok.kind == TokenKind::LParen) {
            isize next_precedence = operator_precedence(tok);
            if (next_precedence <= precedence) {
                break;
            }

            next_token(file);
            Array<AstNode*> arguments = parse_function_arguments(file, arena);
            tok = next_token(file);
            check_condition(tok.kind == TokenKind::RParen, tok, "Expected ')'");

            AstNodeCall* call = AstNodeCall::make(left, arguments, tok, arena);
            left = call;
            continue;
        }

        // Array access
        if (tok.kind == TokenKind::LBracket) {
            isize next_precedence = operator_precedence(tok);
            if (next_precedence <= precedence) {
                break;
            }

            next_token(file);
            AstNode* index = parse_expression(file, true, arena);
            Token next = next_token(file);
            check_condition(next.kind == TokenKind::RBracket, next,
                            "Expected ']'");

            AstNodeBinary* binary =
                AstNodeBinary::make(left, index, tok, arena);
            left = binary;
            continue;
        }

        if (!is_binary_operator(tok)) {
            break;
        }

        isize next_precedence = operator_precedence(tok);
        if (next_precedence <= precedence) {
            break;
        }

        tok = next_token(file);
        AstNode* right =
            parse_expression_rec(file, next_precedence, allow_newlines, arena);
        AstNode* binary = AstNodeBinary::make(left, right, tok, arena);
        left = binary;
    }

    return left;
}

AstNode* parse_expression(AstFile* file, bool allow_newlines, Arena* arena) {
    return parse_expression_rec(file, 0, allow_newlines, arena);
}

AstNode* parse_declaration(AstFile* file, Arena* arena) {
    Token tok = next_token(file);
    check_condition(tok.kind == TokenKind::Identifier, tok,
                    "Expected an identifier");
    AstNodeIdentifier* name = AstNodeIdentifier::make(tok, arena);

    Token colon = next_token(file);
    check_condition(colon.kind == TokenKind::Colon, colon, "Expected ':'");

    AstNode* type = nullptr;
    tok = next_token(file);
    if (tok.kind != TokenKind::Assign && tok.kind != TokenKind::Colon) {
        type = parse_type(file, arena);
        tok = next_token(file);
    }

    if (tok.kind == TokenKind::Colon) {
        AstNode* value = parse_expression(file, false, arena);

        return AstNodeDeclaration::make(name, type, value,
                                        AstDeclarationKind::Constant, arena);
    } else if (tok.kind == TokenKind::Assign) {
        AstNode* value = parse_expression(file, false, arena);

        return AstNodeDeclaration::make(name, type, value,
                                        AstDeclarationKind::Variable, arena);
    }

    check_condition(false, tok, "Expected ':' or '='");
}

bool ast_node_is_assignable(AstNode* node) {
    if (node == nullptr) {
        // We return true if an error has occured (we don't have the node)
        // to reduce the number of error messages.
        return true;
    }

    switch (node->kind) {
    case AstNodeKind::Identifier:
        return true;
    case AstNodeKind::Binary: {
        AstNodeBinary* binary = node->as_binary();
        switch (binary->op) {
        case TokenKind::Period: {
            bool left = ast_node_is_assignable(binary->left);
            bool right = ast_node_is_assignable(binary->right);
            return left && right;
        }
        case TokenKind::LBracket:
            return ast_node_is_assignable(binary->left);
        default:
            return false;
        }
    }
    default:
        return false;
    }
}

AstNode* parse_statement(AstFile* file, Arena* arena) {
    core_assert(file);
    core_assert(arena);

    skip_newlines(file);
    Token tok = peek_token(file);

    switch (tok.kind) {
    case TokenKind::Break: {
        next_token(file);
        AstNode* value = parse_expression(file, false, arena);
        return AstNodeBreak::make(value, tok, arena);
    }
    case TokenKind::Continue: {
        next_token(file);
        return AstNodeContinue::make(tok, arena);
    }
    case TokenKind::LBrace: {
        return parse_block(file, arena);
    }
    case TokenKind::Identifier: {
        if (peek_token(file, 2).kind == TokenKind::Colon) {
            return parse_declaration(file, arena);
        }
        /* fallthrough */
    }
    default: {
        AstNode* expr = parse_expression(file, false, arena);
        Token next = peek_token(file);

        switch (next.kind) {
        case TokenKind::Assign: {
            next_token(file);
            AstNode* value = parse_expression(file, false, arena);
            check_condition(ast_node_is_assignable(expr), next,
                            "Left-hand side of assignment must be assignable");
            return AstNodeAssignment::make(expr, value, next, arena);
        }
        case TokenKind::Newline: {
            next_token(file);
            return expr;
        }
        case TokenKind::RBrace: {
            return expr;
        }
        case TokenKind::Eof: {
            return expr;
        }
        default:
            next_token(file);
            check_condition(false, next, "Expected a statement");
        }
    }
    }
}

void ast_file_parse(AstFile* file, Arena* arena) {
    Token tok = peek_token(file);
    skip_newlines(file);
    while (tok.kind != TokenKind::Eof) {
        AstNode* declaration = parse_declaration(file, arena);
        array_push(&file->ast->declarations, declaration);
        skip_newlines(file);
        tok = peek_token(file);
    }
}

/// ------------------
/// Errors
/// ------------------

Array<String> parse_error_pretty_print(ParseError* error, TokenLocator* locator,
                                       Arena* arena) {
    Array<String> parts;
    array_init(&parts, 8, arena);

    TokenPos pos = token_locator_pos(locator, error->token);
    String line = token_locator_get_line(locator, pos.line);

    char buffer[1024] = {};
    snprintf(buffer, 1024, "     ┌─ Error at line: %ld on column %ld\n",
             pos.line, pos.column);
    String first_line = string_from_cstr_alloc(buffer, arena);
    array_push(&parts, first_line);

    snprintf(buffer, 1024, "%5ld│ ", pos.line);
    String line_number = string_from_cstr_alloc(buffer, arena);
    array_push(&parts, line_number);

    array_push(&parts, line);

    char* caret_line = arena_alloc<char>(arena, error->token.source.size + 1);
    memset(caret_line, '^', error->token.source.size);

    snprintf(buffer, 1024, "\n     │ %*c%s\n", (int)pos.column, ' ',
             caret_line);
    String caret = string_from_cstr_alloc(buffer, arena);
    array_push(&parts, caret);

    snprintf(buffer, 1024, "     └─ %.*s\n", (int)error->message.size,
             error->message.data);
    String message = string_from_cstr_alloc(buffer, arena);
    array_push(&parts, message);

    return parts;
}
