#include "core.hpp"
#include <ostream>

enum class TokenKind {
    Eof,
    Newline,
    Identifier,
    Invalid,

    // Keywords
    Func,
    If,
    Else,
    For,
    Break,
    Continue,

    // Literals
    Integer, // 1, 2, 3, ...
    String,  // "hello", "world", ...

    // Operators
    Plus,         // +
    Minus,        // -
    Asterisk,     // *
    Slash,        // /
    LessThan,     // <
    LessEqual,    // <=
    GreaterThan,  // >
    GreaterEqual, // >=
    Equal,        // ==
    NotEqual,     // !=
    BinaryAnd,    // &
    BinaryOr,     // |
    LogicalAnd,   // &&
    LogicalOr,    // ||
    Bang,         // !
    Period,       // .
    Assign,       // =

    // Other
    Arrow,     // ->
    Comma,     // ,
    Colon,     // :
    LParen,    // (
    RParen,    // )
    LBrace,    // {
    RBrace,    // }
    LBracket,  // [
    RBracket,  // ]
    Semicolon, // ;

};

struct Token {
    TokenKind kind;
    String source;
};

struct Tokenizer {
    String source;
    isize position;
    isize read_position;
};

enum class TokenizerErrorKind {
    None,
    UnclosedString,
    InvalidCharacter,
};

// NOTE(juraj): Only used for gtest to print TokenizerErrorKind
inline std::ostream& operator<<(std::ostream& os, TokenizerErrorKind kind) {
    switch (kind) {
    case TokenizerErrorKind::None: {
        os << "None";
        break;
    }
    case TokenizerErrorKind::UnclosedString: {
        os << "UnclosedString";
        break;
    }
    case TokenizerErrorKind::InvalidCharacter: {
        os << "InvalidCharacter";
        break;
    }
    }

    return os;
}

struct TokenizerResult {
    TokenizerErrorKind error;
    Token token;
};

void tokenizer_init(Tokenizer* tokenizer, String source);
TokenizerResult tokenizer_next_token(Tokenizer* tokenizer);

// Only used for gtest to print TokenKind
inline std::ostream& operator<<(std::ostream& os, TokenKind kind) {
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
    }
    return os;
}
