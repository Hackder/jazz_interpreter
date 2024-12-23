#include "ast.hpp"
#include "core.hpp"
#include "token_pos.hpp"

struct ParseError {
    Token token;
    String message;
};

Array<String> parse_error_pretty_print(ParseError* error, TokenLocator* locator,
                                       Arena* arena);

struct AstFile {
    Tokenizer tokenizer;
    RingBuffer<Token> tokens;
    Array<ParseError> errors;
    Ast* ast;
};

inline void ast_file_init(AstFile* file, Tokenizer tokenizer,
                          isize peek_capacity, Arena* arena) {
    file->tokenizer = tokenizer;
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
