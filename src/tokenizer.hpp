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

    // Literals
    Integer, // 1, 2, 3, ...
    String,  // "hello", "world", ...

    // Operators
    LParen,       // (
    RParen,       // )
    LBrace,       // {
    RBrace,       // }
    LBracket,     // [
    RBracket,     // ]
    Assign,       // =
    Colon,        // :
    Period,       // .
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

// Only used for gtest to print TokenizerErrorKind
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

void tokenzier_init(Tokenizer* tokenizer, String source);
TokenizerResult tokenizer_next_token(Tokenizer* tokenizer);
