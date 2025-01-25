#include "compiler.hpp"
#include "ast.hpp"
#include "core.hpp"

struct CompilerContext {
    Arena* arena;
    Array<Slice<Inst>> functions;
    Array<u8> static_data;
    HashMap<String, isize> function_name_offset;
};

template <typename T>
isize ctx_push_static_data(CompilerContext* ctx, T value) {
    isize offset = ctx->static_data.size;
    u8* data = (u8*)&value;
    for (isize i = 0; i < sizeof(T); i++) {
        array_push(&ctx->static_data, data[i]);
    }
    return offset;
}

i64 string_parse_to_i64(String str) {
    core_assert(str.size > 0);

    i64 result = 0;
    bool negative = false;
    if (str[0] == '-') {
        negative = true;
        str.data += 1;
        str.size -= 1;
    }

    for (isize i = 0; i < str.size; i++) {
        core_assert(str.data[i] >= '0' && str.data[i] <= '9');
        if (result > (isize)INT_MAX / 10) {
            core_assert_msg(false,
                            "Integer too large to be represendted by i64");
        }
        result *= 10;
        i64 digit = str.data[i] - '0';

        if (result > (isize)INT_MAX - digit) {
            core_assert_msg(false,
                            "Integer too large to be represendted by i64");
        }
        result += digit;
    }

    if (negative) {
        result = -result;
    }

    return result;
}

CodeUnit ast_compile_to_bytecode(Ast* ast, Arena* arena) {
    Array<Slice<Inst>> functions = {};
    array_init(&functions, ast->declarations.size, arena);

    Array<u8> static_data = {};
    array_init(&static_data, 1024, arena);

    HashMap<String, isize> function_name_offset = {};
    hash_map_init(&function_name_offset, ast->declarations.size, arena);
    hash_map_insert_or_set(&function_name_offset, string_from_cstr("main"),
                           (isize)0);

    CompilerContext ctx = {
        .arena = arena,
        .functions = functions,
        .static_data = static_data,
        .function_name_offset = function_name_offset,
    };

    // Do a first pass, where we register all the functions and all the
    // constants. This must be done, as we need to know the function offsets of
    // other functions, when we generate the bytecode for any function. (and we
    // support out of order definitions)
    isize next_function_offset = 1;
    for (isize i = 0; i < ast->declarations.size; i++) {
        AstNode* node = ast->declarations[i];
        AstNodeDeclaration* decl = node->as_declaration();
        AstNodeLiteral* name = decl->name->as_literal();
        AstNode* value = decl->value;
        Type* type = type_set_get_single(value->type_set);

        switch (type->kind) {
        case TypeKind::Integer: {
            AstNodeLiteral* literal = value->as_literal();
            core_assert(literal->literal_kind == AstLiteralKind::Integer);
            i64 value = string_parse_to_i64(literal->token.source);
            isize offset = ctx_push_static_data(&ctx, value);
            literal->static_data_offset = offset;
            break;
        }
        case TypeKind::Float: {
            // TODO(juraj): implement float
            core_assert(false);
            break;
        }
        case TypeKind::String: {
            // TODO(juraj): implement string
            core_assert(false);
            break;
        }
        case TypeKind::Bool: {
            // TODO(juraj): implement bool
            core_assert(false);
            break;
        }
        case TypeKind::Function: {
            if (name->token.source == string_from_cstr("main")) {
                break;
            }

            hash_map_insert_or_set(&ctx.function_name_offset,
                                   name->token.source, next_function_offset);
            next_function_offset += 1;
            break;
        }
        case TypeKind::Void: {
            core_assert(false);
            break;
        }
        }
    }

    CodeUnit code = {
        .static_data = slice_from_array(&static_data),
        .functions = slice_from_array(&functions),
    };

    return code;
}
