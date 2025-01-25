#include "compiler.hpp"
#include "ast.hpp"
#include "bytecode.hpp"
#include "core.hpp"

struct CompilerContext {
    Arena* arena;
    Array<Slice<Inst>> functions;
    Array<u8> static_data;
    HashMap<String, isize> function_name_offset_map;
};

template <typename T>
isize ctx_push_static_data(CompilerContext* ctx, T value) {
    isize offset = ctx->static_data.size;
    u8* data = (u8*)&value;
    for (isize i = 0; i < (isize)sizeof(T); i++) {
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

void compile_block(CompilerContext* ctx, AstNodeBlock* block,
                   Array<Inst>* instructions) {
    for (isize i = 0; i < block->statements.size; i++) {
        // TODO: implement
    }
}

void compile_function(CompilerContext* ctx, AstNodeFunction* function,
                      isize function_offset) {
    Array<Inst> instructions = {};
    array_init(&instructions, 32, ctx->arena);

    isize offset = 0;
    for (isize i = function->parameters.size - 1; i >= 0; i--) {
        AstNodeParameter* param = function->parameters[i];
        Type* type = type_set_get_single(param->type_set);
        offset -= type->size;
        param->name->ptr = mem_ptr_stack_rel(offset);
    }

    compile_block(ctx, function->body, &instructions);
    array_push(&instructions, inst_return());

    ctx->functions[function_offset] = slice_from_array(&instructions);
}

void add_init_function(CompilerContext* ctx) {
    isize main_function_offset = hash_map_must_get(
        &ctx->function_name_offset_map, string_from_cstr("main"));

    Inst instructions[] = {
        inst_call(main_function_offset),
        inst_exit(0),
    };

    ctx->functions[0] = slice_from_inline_alloc(instructions, ctx->arena);
}

CodeUnit ast_compile_to_bytecode(Ast* ast, Arena* arena) {
    Array<Slice<Inst>> functions = {};
    array_init(&functions, ast->declarations.size, arena);
    array_push(&functions, Slice<Inst>{});

    Array<u8> static_data = {};
    array_init(&static_data, 1024, arena);

    HashMap<String, isize> function_name_offset = {};
    hash_map_init(&function_name_offset, ast->declarations.size, arena);

    CompilerContext ctx = {
        .arena = arena,
        .functions = functions,
        .static_data = static_data,
        .function_name_offset_map = function_name_offset,
    };

    // Do a first pass, where we register all the functions and all the
    // constants. This must be done, as we need to know the function offsets of
    // other functions, when we generate the bytecode for any function. (and we
    // support out of order definitions)
    isize next_function_offset = 1;
    for (isize i = 0; i < ast->declarations.size; i++) {
        AstNode* node = ast->declarations[i];
        AstNodeDeclaration* decl = node->as_declaration();
        AstNodeIdentifier* name = decl->name->as_identifier();
        AstNode* value = decl->value;
        Type* type = type_set_get_single(value->type_set);

        switch (type->kind) {
        case TypeKind::Integer: {
            AstNodeLiteral* literal = value->as_literal();
            core_assert(literal->literal_kind == AstLiteralKind::Integer);
            i64 value = string_parse_to_i64(literal->token.source);
            isize offset = ctx_push_static_data(&ctx, value);
            literal->static_data_ptr = mem_ptr_static_data(offset);
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
            hash_map_insert_or_set(&ctx.function_name_offset_map,
                                   name->token.source, next_function_offset);
            next_function_offset += 1;
            array_push(&ctx.functions, Slice<Inst>{});
            break;
        }
        case TypeKind::Void: {
            core_assert(false);
            break;
        }
        }
    }

    for (isize i = 0; i < ast->declarations.size; i++) {
        AstNode* node = ast->declarations[i];
        AstNodeDeclaration* decl = node->as_declaration();
        AstNodeIdentifier* name = decl->name->as_identifier();
        AstNode* value = decl->value;
        Type* type = type_set_get_single(value->type_set);

        switch (type->kind) {
        case TypeKind::Function: {
            isize function_offset = hash_map_must_get(
                &ctx.function_name_offset_map, name->token.source);
            compile_function(&ctx, value->as_function(), function_offset);
        }
        case TypeKind::Integer:
        case TypeKind::Float:
        case TypeKind::String:
        case TypeKind::Bool: {
            continue;
        }
        case TypeKind::Void: {
            core_assert(false);
            break;
        }
        }
    }

    add_init_function(&ctx);

    CodeUnit code = {
        .static_data = slice_from_array(&ctx.static_data),
        .functions = slice_from_array(&ctx.functions),
    };

    return code;
}
