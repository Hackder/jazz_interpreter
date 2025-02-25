#include "tokenizer.hpp"
#include "core.hpp"

void tokenizer_init(Tokenizer* tokenizer, String source) {
    tokenizer->source = source;
    tokenizer->position = 0;
    tokenizer->read_position = 0;
}

void skip_whitespace(Tokenizer* tokenizer) {
    isize last_newline = -1;
    while (tokenizer->read_position < tokenizer->source.size) {
        char c = tokenizer->source[tokenizer->read_position];
        if (c == ' ' || c == '\t' || c == '\r') {
            tokenizer->read_position += 1;
            continue;
        }

        if (c == '\n') {
            last_newline = tokenizer->read_position;
            tokenizer->read_position += 1;
            continue;
        }

        break;
    }

    if (last_newline != -1) {
        tokenizer->read_position = last_newline;
    }

    tokenizer->position = tokenizer->read_position;
}

void try_read_string(Tokenizer* tokenizer, TokenizerResult* result) {
    core_assert(tokenizer->read_position >= tokenizer->position);
    core_assert(tokenizer->read_position <= tokenizer->source.size);

    result->token.kind = TokenKind::String;
    while (tokenizer->read_position < tokenizer->source.size) {
        if (tokenizer->source[tokenizer->read_position] == '"') {
            break;
        }

        tokenizer->read_position += 1;
    }

    if (tokenizer->read_position >= tokenizer->source.size) {
        result->error = TokenizerErrorKind::UnclosedString;
        result->token.source =
            string_substr(tokenizer->source, tokenizer->position,
                          tokenizer->read_position - tokenizer->position);
        return;
    }

    result->token.source =
        string_substr(tokenizer->source, tokenizer->position + 1,
                      tokenizer->read_position - tokenizer->position - 1);

    tokenizer->read_position += 1;
}

void try_read_number(Tokenizer* tokenizer, TokenizerResult* result) {
    core_assert(tokenizer->read_position >= tokenizer->position);
    core_assert(tokenizer->read_position <= tokenizer->source.size);

    result->token.kind = TokenKind::Integer;
    while (tokenizer->read_position < tokenizer->source.size) {
        char c = tokenizer->source[tokenizer->read_position];
        if (c < '0' || c > '9') {
            break;
        }

        tokenizer->read_position += 1;
    }

    result->token.source =
        string_substr(tokenizer->source, tokenizer->position,
                      tokenizer->read_position - tokenizer->position);
}

void try_read_identifier(Tokenizer* tokenizer, TokenizerResult* result) {
    result->token.kind = TokenKind::Identifier;
    while (tokenizer->read_position < tokenizer->source.size) {
        char c = tokenizer->source[tokenizer->read_position];
        bool isLowerAlpha = c >= 'a' && c <= 'z';
        bool isUpperAlpha = c >= 'A' && c <= 'Z';
        bool isDigit = c >= '0' && c <= '9';
        bool isSpecial = c == '_';
        if (!isLowerAlpha && !isUpperAlpha && !isDigit && !isSpecial) {
            break;
        }

        tokenizer->read_position += 1;
    }

    result->token.source =
        string_substr(tokenizer->source, tokenizer->position,
                      tokenizer->read_position - tokenizer->position);
}

TokenizerResult tokenizer_next_token(Tokenizer* tokenizer) {
    core_assert(tokenizer->position <= tokenizer->source.size);
    core_assert(tokenizer->read_position == tokenizer->position);

    skip_whitespace(tokenizer);
    if (tokenizer->position >= tokenizer->source.size) {
        return TokenizerResult{.error = TokenizerErrorKind::None,
                               .token = Token{
                                   .kind = TokenKind::Eof,
                                   .source = {.data = tokenizer->source.data +
                                                      tokenizer->source.size,
                                              .size = 0},
                               }};
    }

    TokenizerResult result = {};
    result.error = TokenizerErrorKind::None;
    result.token.kind = TokenKind::Invalid;
    result.token.source =
        string_substr(tokenizer->source, tokenizer->read_position, 1);

    char c = tokenizer->source[tokenizer->read_position];
    tokenizer->read_position += 1;

    switch (c) {
    case '\n': {
        result.token.kind = TokenKind::Newline;
        break;
    }
    case '(': {
        result.token.kind = TokenKind::LParen;
        break;
    }
    case ')': {
        result.token.kind = TokenKind::RParen;
        break;
    }
    case '{': {
        result.token.kind = TokenKind::LBrace;
        break;
    }
    case '}': {
        result.token.kind = TokenKind::RBrace;
        break;
    }
    case '[': {
        result.token.kind = TokenKind::LBracket;
        break;
    }
    case ']': {
        result.token.kind = TokenKind::RBracket;
        break;
    }
    case '=': {
        if (tokenizer->read_position < tokenizer->source.size &&
            tokenizer->source[tokenizer->read_position] == '=') {
            result.token.kind = TokenKind::Equal;
            result.token.source.size = 2;
            tokenizer->read_position += 1;
            break;
        }

        result.token.kind = TokenKind::Assign;
        break;
    }
    case ':': {
        result.token.kind = TokenKind::Colon;
        break;
    }
    case '"': {
        try_read_string(tokenizer, &result);
        break;
    }
    case '.': {
        result.token.kind = TokenKind::Period;
        break;
    }
    case '+': {
        result.token.kind = TokenKind::Plus;
        break;
    }
    case '-': {
        if (tokenizer->read_position < tokenizer->source.size &&
            tokenizer->source[tokenizer->read_position] == '>') {
            result.token.kind = TokenKind::Arrow;
            result.token.source.size = 2;
            tokenizer->read_position += 1;
            break;
        }

        result.token.kind = TokenKind::Minus;
        break;
    }
    case '*': {
        result.token.kind = TokenKind::Asterisk;
        break;
    }
    case '/': {
        result.token.kind = TokenKind::Slash;
        break;
    }
    case ',': {
        result.token.kind = TokenKind::Comma;
        break;
    }
    case ';': {
        result.token.kind = TokenKind::Semicolon;
        break;
    }
    case '<': {
        if (tokenizer->read_position < tokenizer->source.size &&
            tokenizer->source[tokenizer->read_position] == '=') {
            result.token.kind = TokenKind::LessEqual;
            result.token.source.size = 2;
            tokenizer->read_position += 1;
            break;
        }
        result.token.kind = TokenKind::LessThan;
        break;
    }
    case '>': {
        if (tokenizer->read_position < tokenizer->source.size &&
            tokenizer->source[tokenizer->read_position] == '=') {
            result.token.kind = TokenKind::GreaterEqual;
            result.token.source.size = 2;
            tokenizer->read_position += 1;
            break;
        }

        result.token.kind = TokenKind::GreaterThan;
        break;
    }
    case '!': {
        if (tokenizer->read_position < tokenizer->source.size &&
            tokenizer->source[tokenizer->read_position] == '=') {
            result.token.kind = TokenKind::NotEqual;
            result.token.source.size = 2;
            tokenizer->read_position += 1;
            break;
        }

        result.token.kind = TokenKind::Bang;
        break;
    }
    case '&': {
        if (tokenizer->read_position < tokenizer->source.size &&
            tokenizer->source[tokenizer->read_position] == '&') {
            result.token.kind = TokenKind::LogicalAnd;
            result.token.source.size = 2;
            tokenizer->read_position += 1;
            break;
        }

        result.token.kind = TokenKind::BinaryAnd;
        break;
    }
    case '|': {
        if (tokenizer->read_position < tokenizer->source.size &&
            tokenizer->source[tokenizer->read_position] == '|') {
            result.token.kind = TokenKind::LogicalOr;
            result.token.source.size = 2;
            tokenizer->read_position += 1;
            break;
        }

        result.token.kind = TokenKind::BinaryOr;
        break;
    }
    default: {
        if (c >= '0' && c <= '9') {
            try_read_number(tokenizer, &result);
            break;
        }

        bool isLowerAlpha = c >= 'a' && c <= 'z';
        bool isUpperAlpha = c >= 'A' && c <= 'Z';

        if (isLowerAlpha || isUpperAlpha) {
            try_read_identifier(tokenizer, &result);

            if (result.error != TokenizerErrorKind::None) {
                break;
            }

            if (result.token.source == "fn") {
                result.token.kind = TokenKind::Func;
                break;
            }

            if (result.token.source == "if") {
                result.token.kind = TokenKind::If;
                break;
            }

            if (result.token.source == "else") {
                result.token.kind = TokenKind::Else;
                break;
            }

            if (result.token.source == "for") {
                result.token.kind = TokenKind::For;
                break;
            }

            if (result.token.source == "break") {
                result.token.kind = TokenKind::Break;
                break;
            }

            if (result.token.source == "continue") {
                result.token.kind = TokenKind::Continue;
                break;
            }

            if (result.token.source == "true" ||
                result.token.source == "false") {
                result.token.kind = TokenKind::Bool;
                break;
            }

            if (result.token.source == "return") {
                result.token.kind = TokenKind::Return;
                break;
            }

            break;
        }

        result.error = TokenizerErrorKind::InvalidCharacter;
        break;
    }
    }

    tokenizer->position = tokenizer->read_position;
    return result;
}

std::ostream& operator<<(std::ostream& os, TokenKind kind) {
    switch (kind) {
    case TokenKind::Eof:
        os << "Eof";
        break;
    case TokenKind::Newline:
        os << "Newline";
        break;
    case TokenKind::Identifier:
        os << "Identifier";
        break;
    case TokenKind::Invalid:
        os << "Invalid";
        break;
    case TokenKind::Func:
        os << "Func";
        break;
    case TokenKind::If:
        os << "If";
        break;
    case TokenKind::Else:
        os << "Else";
        break;
    case TokenKind::For:
        os << "For";
        break;
    case TokenKind::Integer:
        os << "Integer";
        break;
    case TokenKind::String:
        os << "String";
        break;
    case TokenKind::LParen:
        os << "LParen";
        break;
    case TokenKind::RParen:
        os << "RParen";
        break;
    case TokenKind::LBrace:
        os << "LBrace";
        break;
    case TokenKind::RBrace:
        os << "RBrace";
        break;
    case TokenKind::LBracket:
        os << "LBracket";
        break;
    case TokenKind::RBracket:
        os << "RBracket";
        break;
    case TokenKind::Assign:
        os << "Assign";
        break;
    case TokenKind::Colon:
        os << "Colon";
        break;
    case TokenKind::Period:
        os << "Period";
        break;
    case TokenKind::Plus:
        os << "Plus";
        break;
    case TokenKind::Minus:
        os << "Minus";
        break;
    case TokenKind::Asterisk:
        os << "Asterisk";
        break;
    case TokenKind::Slash:
        os << "Slash";
        break;
    case TokenKind::LessThan:
        os << "LessThan";
        break;
    case TokenKind::LessEqual:
        os << "LessEqual";
        break;
    case TokenKind::GreaterThan:
        os << "GreaterThan";
        break;
    case TokenKind::GreaterEqual:
        os << "GreaterEqual";
        break;
    case TokenKind::Equal:
        os << "Equal";
        break;
    case TokenKind::NotEqual:
        os << "NotEqual";
        break;
    case TokenKind::BinaryAnd:
        os << "BinaryAnd";
        break;
    case TokenKind::BinaryOr:
        os << "BinaryOr";
        break;
    case TokenKind::LogicalAnd:
        os << "LogicalAnd";
        break;
    case TokenKind::LogicalOr:
        os << "LogicalOr";
        break;
    case TokenKind::Bang:
        os << "Bang";
        break;
    case TokenKind::Comma:
        os << "Comma";
        break;
    case TokenKind::Arrow:
        os << "Arrow";
        break;
    case TokenKind::Semicolon:
        os << "Semicolon";
        break;
    case TokenKind::Break:
        os << "Break";
        break;
    case TokenKind::Continue:
        os << "Continue";
        break;
    case TokenKind::Bool:
        os << "Bool";
        break;
    case TokenKind::Return:
        os << "Return";
        break;
    }
    return os;
}
