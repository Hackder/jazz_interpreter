#include "compiler.hpp"
#include "ast.hpp"
#include "bytecode.hpp"
#include "core.hpp"
#include "tokenizer.hpp"

struct CompilerContext {
    Arena* arena;
    Array<Slice<Inst>> functions;
    Array<u8> static_data;
    HashMap<String, isize> function_name_offset_map;
    isize stack_frame_size;
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

bool string_parse_to_bool(String str) {
    if (str == string_from_cstr("true")) {
        return true;
    } else if (str == string_from_cstr("false")) {
        return false;
    } else {
        core_assert(false);
        return false;
    }
}

void compile_block(CompilerContext* ctx, AstNodeBlock* block,
                   Array<Inst>* instructions);

void compile_expression(CompilerContext* ctx, AstNode* expression,
                        Array<Inst>* instructions) {
    switch (expression->kind) {
    case AstNodeKind::Literal: {
        AstNodeLiteral* literal = expression->as_literal();
        Type* type = type_set_get_single(literal->type_set);
        switch (literal->literal_kind) {
        case AstLiteralKind::Integer: {
            i64 value = string_parse_to_i64(literal->token.source);
            isize offset = ctx_push_static_data(ctx, value);
            core_assert(type->size == sizeof(i64));
            Inst push = inst_push_stack(type->size);
            array_push(instructions, push);
            Inst mov = inst_mov(mem_ptr_stack_rel(ctx->stack_frame_size),
                                mem_ptr_static_data(offset), type->size);
            array_push(instructions, mov);
            ctx->stack_frame_size += push.push_stack.size;
            break;
        }
        case AstLiteralKind::Float:
        case AstLiteralKind::String:
            break;
        case AstLiteralKind::Bool: {
            bool value = string_parse_to_bool(literal->token.source);
            isize offset = ctx_push_static_data(ctx, value);
            core_assert(type->size == sizeof(bool));
            Inst push = inst_push_stack(type->size);
            array_push(instructions, push);
            Inst mov = inst_mov(mem_ptr_stack_rel(ctx->stack_frame_size),
                                mem_ptr_static_data(offset), type->size);
            array_push(instructions, mov);
            ctx->stack_frame_size += push.push_stack.size;
            break;
        }
        }
        break;
    }
    case AstNodeKind::Identifier: {
        AstNodeIdentifier* ident = expression->as_identifier();
        Type* type = type_set_get_single(ident->type_set);
        Inst push = inst_push_stack(type->size);
        array_push(instructions, push);
        Inst mov = inst_mov(mem_ptr_stack_rel(ctx->stack_frame_size),
                            ident->ptr, type->size);
        array_push(instructions, mov);
        ctx->stack_frame_size += push.push_stack.size;
        break;
    }
    case AstNodeKind::Binary: {
        AstNodeBinary* binary = expression->as_binary();
        Type* type = type_set_get_single(binary->type_set);
        isize before_left = ctx->stack_frame_size;
        Type* left_type = type_set_get_single(binary->left->type_set);
        compile_expression(ctx, binary->left, instructions);
        isize before_right = ctx->stack_frame_size;
        Type* right_type = type_set_get_single(binary->right->type_set);
        compile_expression(ctx, binary->right, instructions);

        core_assert(left_type->kind == right_type->kind);

        BinOperand op = BinOperand::Int_Add;
        switch (left_type->kind) {
        case TypeKind::Integer: {
            switch (binary->op) {
            case TokenKind::Plus: {
                op = BinOperand::Int_Add;
                break;
            }
            case TokenKind::Minus: {
                op = BinOperand::Int_Sub;
                break;
            }
            case TokenKind::Asterisk: {
                op = BinOperand::Int_Mul;
                break;
            }
            case TokenKind::Slash: {
                op = BinOperand::Int_Div;
                break;
            }
            case TokenKind::BinaryOr: {
                op = BinOperand::Int_BinaryOr;
                break;
            }
            case TokenKind::BinaryAnd: {
                op = BinOperand::Int_BinaryAnd;
                break;
            }
            case TokenKind::Equal: {
                op = BinOperand::Int_Equal;
                break;
            }
            case TokenKind::NotEqual: {
                op = BinOperand::Int_NotEqual;
                break;
            }
            default: {
                core_assert(false);
                break;
            }
            }
            break;
        }
        case TypeKind::Float:
        case TypeKind::String:
        case TypeKind::Bool:
        case TypeKind::Void:
        case TypeKind::Function: {
            core_assert(false);
            break;
        }
        }

        Inst binary_op = inst_binary_op(op, mem_ptr_stack_rel(before_left),
                                        mem_ptr_stack_rel(before_left),
                                        mem_ptr_stack_rel(before_right));
        array_push(instructions, binary_op);

        isize delta = ctx->stack_frame_size - before_left + type->size;
        Inst pop = inst_pop_stack(delta);
        array_push(instructions, pop);
        ctx->stack_frame_size -= delta;

        break;
    }
    case AstNodeKind::Unary:
    case AstNodeKind::Call:
    case AstNodeKind::If:
    case AstNodeKind::For:
    case AstNodeKind::Break:
    case AstNodeKind::Continue:
    case AstNodeKind::Return:
    case AstNodeKind::Block:
    case AstNodeKind::Parameter:
    case AstNodeKind::Function:
    case AstNodeKind::Declaration:
    case AstNodeKind::Assignment: {
        core_assert(false);
        break;
    }
    }
}

void compile_statement(CompilerContext* ctx, AstNode* statement,
                       Array<Inst>* instructions) {
    switch (statement->kind) {
    case AstNodeKind::Literal:
    case AstNodeKind::Identifier:
    case AstNodeKind::Binary:
    case AstNodeKind::Unary:
    case AstNodeKind::Call:
    case AstNodeKind::If:
    case AstNodeKind::For:
    case AstNodeKind::Break:
    case AstNodeKind::Continue:
    case AstNodeKind::Return:
    case AstNodeKind::Block: {
        compile_block(ctx, statement->as_block(), instructions);
        break;
    }
    case AstNodeKind::Declaration: {
        // We are declaring a variable on the stack
        AstNodeDeclaration* decl = statement->as_declaration();
        decl->name->ptr = mem_ptr_stack_rel(ctx->stack_frame_size);
        // array_push(instructions, inst_push_stack(type->size));

        // TODO(juraj): Evaluate the expression on the right, the
        // result will be stored on the top of the stack
        compile_expression(ctx, decl->value, instructions);

        break;
    }
    case AstNodeKind::Assignment: {
        AstNodeAssignment* assign = statement->as_assignment();
        AstNodeIdentifier* name = assign->name->as_identifier();
        Type* type = type_set_get_single(assign->name->type_set);

        // Evaluate the expression on the right, the result will be stored
        // on the top of the stack

        // Move the result from the top of the stack to the memory location
        Inst mov = inst_mov(name->ptr, mem_ptr_stack_rel(ctx->stack_frame_size),
                            type->size);
        array_push(instructions, mov);

        ctx->stack_frame_size -= type->size;

        break;
    }
    case AstNodeKind::Parameter:
    case AstNodeKind::Function: {
        core_assert(false);
        break;
    }
    }
}

void compile_block(CompilerContext* ctx, AstNodeBlock* block,
                   Array<Inst>* instructions) {
    for (isize i = 0; i < block->statements.size; i++) {
        compile_statement(ctx, block->statements[i], instructions);
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

    ctx->stack_frame_size = 0;

    compile_block(ctx, function->body, &instructions);

    Inst pop_stack = inst_pop_stack(ctx->stack_frame_size);
    array_push(&instructions, pop_stack);
    if (instructions.size == 0 ||
        instructions[instructions.size - 1].type != InstType::Return) {
        array_push(&instructions, inst_return());
    }

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
        .stack_frame_size = 0,
    };

    // Do a first pass, where we register all the functions and all the
    // constants. This must be done, as we need to know the function offsets
    // of other functions, when we generate the bytecode for any function.
    // (and we support out of order definitions)
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
