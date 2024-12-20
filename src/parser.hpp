#include "ast.hpp"

struct AstFile {
    Tokenizer tokenizer;
    Token peeked_token;
};

AstNode* parse_expression(AstFile* file, Arena* arena);
AstNode* parse_declaration(AstFile* file, Arena* arena);
Ast* ast_file_parse(AstFile* file);
