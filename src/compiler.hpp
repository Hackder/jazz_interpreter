#include "ast.hpp"
#include "bytecode.hpp"
#include "core.hpp"

CodeUnit ast_compile_to_bytecode(Ast* ast, bool optimize, Arena* arena);
