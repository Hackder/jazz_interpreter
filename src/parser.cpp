#include "parser.hpp"
#include "core.hpp"
#include <cstdio>

#define record_parse_depth(file)                                               \
    file->parse_depth += 1;                                                    \
    defer(file->parse_depth -= 1);                                             \
    if (file->parse_depth > 100) {                                             \
        report_error(file, peek_token(file), "Stack overflow",                 \
                     "The parser has reached its maximum recursion depth. "    \
                     "You are probably doing something nasty");                \
        return nullptr;                                                        \
    }

#define report_error_if(cond, token, message, detail)                          \
    if (cond) {                                                                \
        report_error(file, token, message, detail);                            \
        return nullptr;                                                        \
    }

void report_error(AstFile* file, Token token, const char* message,
                  const char* detail) {
    ParseError error = {
        .token = token,
        .message = string_from_cstr(message),
        .detail = string_from_cstr(detail),
    };

    array_push(&file->errors, error);
}

Token peek_token(AstFile* file, isize index = 1) {
    core_assert(index > 0);

    file->consequent_peeks += 1;
    // Detect infinite loops within the parser
    core_assert(file->consequent_peeks <= 10'000);

    bool reported_error = false;

    while (file->tokens.size < index) {
        TokenizerResult token = tokenizer_next_token(&file->tokenizer);

        switch (token.error) {
        case TokenizerErrorKind::None: {
            ring_buffer_push_end(&file->tokens, token.token);
            break;
        }
        case TokenizerErrorKind::UnclosedString: {
            if (reported_error) {
                continue;
            }
            reported_error = true;
            report_error(file, token.token, "No closing '\"' for this string",
                         "Unclosed string literal, string literals must start "
                         "and end with '\"\'.");
        }
        case TokenizerErrorKind::InvalidCharacter: {
            if (reported_error) {
                continue;
            }
            reported_error = true;
            report_error(file, token.token, "Invalid character",
                         "This character is not allowed here, maybe a typo?");
        }
        }
    }

    return file->tokens[index - 1];
}

Token next_token(AstFile* file) {
    Token tok = peek_token(file);
    ring_buffer_pop_front(&file->tokens);
    file->consequent_peeks = 0;
    return tok;
}

AstNode* parse_type(AstFile* file, Arena* arena) {
    record_parse_depth(file);
    Token tok = next_token(file);
    if (tok.kind == TokenKind::Identifier) {
        return AstNodeIdentifier::make(tok, arena);
    }

    // TODO(juraj): support for function type literals
    // fn(int, int) -> int
    report_error(file, tok, "Expected a type",
                 "Type must be a single identifier");
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

isize skip_newlines(AstFile* file) {
    isize skipped = 0;
    Token tok = peek_token(file);
    while (tok.kind == TokenKind::Newline) {
        next_token(file);
        skipped += 1;
        tok = peek_token(file);
    }

    return skipped;
}

void skip_to_next_line_or(AstFile* file, TokenKind kind) {
    Token tok = peek_token(file);
    while (tok.kind != TokenKind::Newline && tok.kind != TokenKind::Eof &&
           tok.kind != kind) {
        next_token(file);
        tok = peek_token(file);
    }
}

void skip_to_next_line(AstFile* file) {
    skip_to_next_line_or(file, TokenKind::Eof);
}

AstNodeBlock* parse_block(AstFile* file, Arena* arena) {
    record_parse_depth(file);
    Token tok = next_token(file);
    report_error_if(tok.kind != TokenKind::LBrace, tok, "Expected '{'",
                    "Expected the start of a code block");

    Array<AstNode*> statements;
    array_init<AstNode*>(&statements, 16, arena);

    skip_newlines(file);

    while (true) {
        Token next = peek_token(file);
        if (next.kind == TokenKind::RBrace) {
            break;
        }

        report_error_if(next.kind == TokenKind::Eof, next,
                        "Unexpected end of file",
                        "Source file has ended before the current code block "
                        "was closed. Make sure you are not missing a '}'");

        AstNode* statement = parse_statement(file, arena);
        if (statement == nullptr) {
            skip_to_next_line(file);
        }

        array_push<AstNode*>(&statements, statement);

        next = peek_token(file);
        if (next.kind == TokenKind::RBrace) {
            break;
        }

        isize skipped = skip_newlines(file);
        if (skipped == 0) {
            report_error(
                file, next, "Expected newline",
                "Statements within a code block must be separated by newlines");
            skip_to_next_line_or(file, TokenKind::RBrace);
        }
    }

    tok = next_token(file);
    core_assert(tok.kind == TokenKind::RBrace);

    return AstNodeBlock::make(statements, tok, arena);
}

AstNodeFunction* parse_function_expression(AstFile* file, Arena* arena) {
    record_parse_depth(file);
    Token fn_keyword = next_token(file);
    report_error_if(fn_keyword.kind != TokenKind::Func, fn_keyword,
                    "Unexpected token",
                    "Expected 'fn' the start of a function definition");

    Token tok = next_token(file);
    report_error_if(
        tok.kind != TokenKind::LParen, tok, "Expected '('",
        "Expected a list of function parameters, enclosed in parentheses");

    Array<AstNodeParameter*> parameters;
    array_init(&parameters, 8, arena);

    while (true) {
        Token next = peek_token(file);
        if (next.kind == TokenKind::RParen) {
            break;
        }

        Token name = next_token(file);
        report_error_if(name.kind != TokenKind::Identifier, name,
                        "Invalid parameter list",
                        "Parameter name must be an identifier")

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
    record_parse_depth(file);
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
        report_error_if(tok.kind != TokenKind::RParen, tok, "Expected ')'",
                        "Only one expression can be enclosed in a single pair "
                        "of parentheses. There should be a closing ')' here.");
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

                report_error_if(next.kind == TokenKind::Eof, next,
                                "Unexpected end of file",
                                "After the 'for' definition, there should be a "
                                "block of code enclosed in '{' and '}'");
            }

            if (number_of_semicolons == 2) {
                init = parse_statement(file, arena);
                Token tok = next_token(file);
                report_error_if(
                    tok.kind != TokenKind::Semicolon, tok,
                    "Expected a semicolon here",
                    "The first part of a for loop can be only one statement.");
                condition = parse_expression(file, false, arena);
                tok = next_token(file);
                report_error_if(
                    tok.kind != TokenKind::Semicolon, tok,
                    "Expected a semicolon here",
                    "The second part of a for loop can be only one expression");
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
    case TokenKind::Newline: {
        if (!allow_newlines) {
            report_error(
                file, tok, "Unexpected newline",
                "Newlines are not allowed within this expression. If you want "
                "to split it into multiple lines, enclose it in parentheses.");
            return nullptr;
        }
        /* fallthrough */
    }
    // Invalid
    default: {
        report_error(
            file, tok, "Unexpected token",
            "Expected an expression operant, but found something else.");
        return nullptr;
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
            continue;
        }

        if (tok.kind == TokenKind::RParen) {
            break;
        }

        report_error(file, tok, "Expected ',' or ')'",
                     "When calling a function, the arguments must be separated "
                     "by commas and enclosed in parentheses.");
        return arguments;
    }

    return arguments;
}

AstNode* parse_expression_rec(AstFile* file, isize precedence,
                              bool allow_newlines, Arena* arena) {
    record_parse_depth(file);
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
            report_error_if(tok.kind != TokenKind::RParen, tok,
                            "Expected ')' here",
                            "When calling a function, the arguments must be "
                            "enclosed in parentheses.");

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
            report_error_if(next.kind != TokenKind::RBracket, next,
                            "Expected ']' here",
                            "Array access must be enclosed "
                            "in square brackets.");

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
    record_parse_depth(file);
    return parse_expression_rec(file, 0, allow_newlines, arena);
}

AstNode* parse_declaration(AstFile* file, Arena* arena) {
    record_parse_depth(file);
    Token tok = next_token(file);
    report_error_if(
        tok.kind != TokenKind::Identifier, tok, "Expected an identifier",
        "There should be a declaration name here, which is an identifier. "
        "Declarations are in the for of 'name := value' or 'name :: value'");
    AstNodeIdentifier* name = AstNodeIdentifier::make(tok, arena);

    Token colon = next_token(file);
    report_error_if(colon.kind != TokenKind::Colon, colon, "Expected ':'",
                    "After a declaration name, there should be a ':' which "
                    "signifies that it is a declaration");

    AstNode* type = nullptr;
    tok = peek_token(file);
    if (tok.kind != TokenKind::Assign && tok.kind != TokenKind::Colon) {
        type = parse_type(file, arena);
    }
    tok = next_token(file);

    if (tok.kind == TokenKind::Colon) {
        AstNode* value = parse_expression(file, false, arena);

        return AstNodeDeclaration::make(name, type, value,
                                        AstDeclarationKind::Constant, arena);
    } else if (tok.kind == TokenKind::Assign) {
        AstNode* value = parse_expression(file, false, arena);

        return AstNodeDeclaration::make(name, type, value,
                                        AstDeclarationKind::Variable, arena);
    }

    report_error(file, tok, "Expected ':' or '='",
                 "You can either declare a variable with 'name := value' or a "
                 "constant with 'name :: value'");
    return nullptr;
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
    record_parse_depth(file);

    skip_newlines(file);
    Token tok = peek_token(file);

    switch (tok.kind) {
    case TokenKind::Break: {
        next_token(file);
        AstNode* value = nullptr;
        if (peek_token(file).kind != TokenKind::Newline) {
            value = parse_expression(file, false, arena);
        }
        return AstNodeBreak::make(value, tok, arena);
    }
    case TokenKind::Continue: {
        next_token(file);
        return AstNodeContinue::make(tok, arena);
    }
    case TokenKind::Return: {
        next_token(file);
        AstNode* value = nullptr;
        if (peek_token(file).kind != TokenKind::Newline) {
            value = parse_expression(file, false, arena);
        }
        return AstNodeReturn::make(value, tok, arena);
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
        if (expr == nullptr) {
            next_token(file);
        }

        Token next = peek_token(file);

        switch (next.kind) {
        case TokenKind::Assign: {
            next_token(file);
            AstNode* value = parse_expression(file, false, arena);
            report_error_if(
                ast_node_is_assignable(expr) == false, next,
                "Invalid left-hand side",
                "Left hand side of this assignment is not of a valid form. You "
                "can only assign to a variable, struct field, or array "
                "element. More complex expressions are not allowed.");
            return AstNodeAssignment::make(expr, value, next, arena);
        }
        case TokenKind::Colon: {
            report_error(file, next, "Invalid declaration",
                         "Left hand side of a declaration must be an "
                         "identifier. It can't be a complex expression.");
        }
        case TokenKind::Newline: {
            return expr;
        }
        case TokenKind::RBrace: {
            return expr;
        }
        case TokenKind::Eof: {
            return expr;
        }
        default:
            return expr;
        }
    }
    }
}

void ast_file_parse(AstFile* file, Arena* arena) {
    Token tok = peek_token(file);
    skip_newlines(file);
    while (tok.kind != TokenKind::Eof) {
        AstNode* declaration = parse_declaration(file, arena);
        if (declaration == nullptr) {
            skip_to_next_line(file);
        }
        array_push(&file->ast.declarations, declaration);
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

    snprintf(buffer, 1024, "\n     │ %*c%s %.*s\n", (int)pos.column - 1, ' ',
             caret_line, (int)error->message.size, error->message.data);
    String caret = string_from_cstr_alloc(buffer, arena);
    array_push(&parts, caret);

    snprintf(buffer, 1024, "     └─ %.*s\n", (int)error->detail.size,
             error->detail.data);
    String message = string_from_cstr_alloc(buffer, arena);
    array_push(&parts, message);

    return parts;
}
