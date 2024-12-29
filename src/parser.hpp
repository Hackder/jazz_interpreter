#include "ast.hpp"
#include "core.hpp"
#include "token_pos.hpp"

struct ParseError {
    Token token;
    String message;
    String detail;
};

Array<String> parse_error_pretty_print(ParseError* error, TokenLocator* locator,
                                       Arena* arena);

struct AstFile {
    Tokenizer tokenizer;
    RingBuffer<Token> tokens;
    // Indicates how many times we have peeked tokens in a row,
    // without consuming any. This is used to detect infinite loops
    isize consequent_peeks;
    // Current depth of the parser, used to prevent stack overflows
    isize parse_depth;
    Array<ParseError> errors;
    Ast* ast;
};

inline void ast_file_init(AstFile* file, Tokenizer tokenizer,
                          isize peek_capacity, Arena* arena) {
    file->tokenizer = tokenizer;
    file->consequent_peeks = 0;
    file->parse_depth = 0;
    ring_buffer_init(&file->tokens, peek_capacity, arena);
    array_init(&file->errors, 8, arena);
    file->ast = arena_alloc<Ast>(arena);
    ast_init(file->ast, arena);
}

inline AstFile* ast_file_make(Tokenizer tokenizer, isize peek_capacity,
                              Arena* arena) {
    AstFile* file = arena_alloc<AstFile>(arena);
    ast_file_init(file, tokenizer, peek_capacity, arena);

    return file;
}

AstNode* parse_expression(AstFile* file, bool allow_newlines, Arena* arena);
AstNode* parse_declaration(AstFile* file, Arena* arena);
AstNode* parse_statement(AstFile* file, Arena* arena);

bool ast_file_exhausted(AstFile* file);
void ast_file_parse(AstFile* file, Arena* arena);
