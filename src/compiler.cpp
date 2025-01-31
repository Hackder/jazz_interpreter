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
    Array<MemPtr> return_ptrs;
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

void push_stack(CompilerContext* ctx, isize size, Array<Inst>* instructions) {
    Inst push = inst_push_stack(size);
    array_push(instructions, push);
    ctx->stack_frame_size += size;
}

void pop_stack(CompilerContext* ctx, isize size, Array<Inst>* instructions) {
    Inst pop = inst_pop_stack(size);
    array_push(instructions, pop);
    ctx->stack_frame_size -= size;
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
            push_stack(ctx, type->size, instructions);
            Inst mov =
                inst_mov(mem_ptr_stack_rel(ctx->stack_frame_size - type->size),
                         mem_ptr_static_data(offset), type->size);
            array_push(instructions, mov);
            break;
        }
        case AstLiteralKind::Float:
        case AstLiteralKind::String:
            break;
        case AstLiteralKind::Bool: {
            bool value = string_parse_to_bool(literal->token.source);
            isize offset = ctx_push_static_data(ctx, value);
            core_assert(type->size == sizeof(bool));
            push_stack(ctx, type->size, instructions);
            Inst mov =
                inst_mov(mem_ptr_stack_rel(ctx->stack_frame_size - type->size),
                         mem_ptr_static_data(offset), type->size);
            array_push(instructions, mov);
            break;
        }
        }
        break;
    }
    case AstNodeKind::Identifier: {
        AstNodeIdentifier* ident = expression->as_identifier();
        Type* type = type_set_get_single(ident->type_set);
        push_stack(ctx, type->size, instructions);

        switch (ident->def->kind) {
        case AstNodeKind::Declaration: {
            AstNodeDeclaration* decl = ident->def->as_declaration();
            Inst mov =
                inst_mov(mem_ptr_stack_rel(ctx->stack_frame_size - type->size),
                         decl->name->ptr, type->size);
            array_push(instructions, mov);
            break;
        }
        case AstNodeKind::Parameter: {
            AstNodeParameter* param = ident->def->as_parameter();
            Inst mov =
                inst_mov(mem_ptr_stack_rel(ctx->stack_frame_size - type->size),
                         param->name->ptr, type->size);
            array_push(instructions, mov);
            break;
        }
        default: {
            core_assert(false);
            break;
        }
        }

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
            case TokenKind::LessThan: {
                op = BinOperand::Int_LessThan;
                break;
            }
            case TokenKind::LessEqual: {
                op = BinOperand::Int_LessEqual;
                break;
            }
            case TokenKind::GreaterThan: {
                op = BinOperand::Int_GreaterThan;
                break;
            }
            case TokenKind::GreaterEqual: {
                op = BinOperand::Int_GreaterEqual;
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

        isize extra_stack_space_from_op_args =
            left_type->size + right_type->size - type->size;

        // NOTE(juraj): This doesn't handle the case, where the result
        // is larger than the two operands combined. There is no way
        // this can happen currently. But if you came across this in the
        // future, there is no reason why this couldn't be implemented.
        core_assert(extra_stack_space_from_op_args >= 0);

        pop_stack(ctx, extra_stack_space_from_op_args, instructions);

        break;
    }
    case AstNodeKind::Call: {
        AstNodeCall* call = expression->as_call();
        AstNodeIdentifier* callee_ident = call->callee->as_identifier();
        FunctionType* callee_type =
            type_set_get_single(callee_ident->type_set)->as_function();

        // Calling convension:
        // 1. Push the stack by the size of the return value
        // 2. Push the arguments on the stack
        // 3. Call the function
        // 4. Pop the arguments from the stack
        // 5. Leave the return value on the stack

        isize before_size = ctx->stack_frame_size;

        Type* return_type = type_set_get_single(callee_type->return_type);
        push_stack(ctx, return_type->size, instructions);

        for (isize i = 0; i < call->arguments.size; i++) {
            AstNode* arg = call->arguments[i];
            compile_expression(ctx, arg, instructions);
        }

        switch (callee_ident->def->kind) {
        case AstNodeKind::Declaration: {
            AstNodeDeclaration* decl = callee_ident->def->as_declaration();
            AstNodeFunction* fn = decl->value->as_function();
            if (fn->builtin != nullptr) {
                Inst call_inst = inst_call_builtin(fn->builtin);
                array_push(instructions, call_inst);
            } else {
                isize new_fp = fn->offset;
                Inst call_inst = inst_call(new_fp);
                array_push(instructions, call_inst);
            }
            break;
        }
        case AstNodeKind::Parameter: {
            core_assert_msg(false, "Function pointers are not yet implemented");
            break;
        }
        default: {
            core_assert(false);
            break;
        }
        }

        isize arguments_size =
            ctx->stack_frame_size - before_size - return_type->size;
        pop_stack(ctx, arguments_size, instructions);

        break;
    }
    case AstNodeKind::Unary:
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
    case AstNodeKind::Break:
    case AstNodeKind::Continue: {
        // TODO
        core_assert(false);
        break;
    }
    case AstNodeKind::For: {
        isize before_size = ctx->stack_frame_size;
        AstNodeFor* for_node = statement->as_for();
        compile_statement(ctx, for_node->init, instructions);

        isize for_condition_ip = instructions->size;

        compile_expression(ctx, for_node->condition, instructions);

        isize jump_to_end_index = instructions->size;
        Inst jmp_to_end_temp = inst_jump_if_not(mem_ptr_invalid(), -1);
        array_push(instructions, jmp_to_end_temp);

        pop_stack(ctx, sizeof(bool), instructions);

        compile_block(ctx, for_node->then_branch->as_block(), instructions);

        compile_statement(ctx, for_node->update, instructions);
        Inst jmp_to_condition = inst_jump(for_condition_ip);
        array_push(instructions, jmp_to_condition);

        // Push the condition result back on the stack
        ctx->stack_frame_size += sizeof(bool);

        Inst jmp_to_end = inst_jump_if_not(
            mem_ptr_stack_rel(ctx->stack_frame_size - (isize)sizeof(bool)),
            instructions->size);
        (*instructions)[jump_to_end_index] = jmp_to_end;

        // Finally we pop the variables created in the `init` part
        // of the for loop
        core_assert(ctx->stack_frame_size >= before_size);
        pop_stack(ctx, ctx->stack_frame_size - before_size, instructions);

        break;
    }
    case AstNodeKind::If: {
        AstNodeIf* if_node = statement->as_if();
        isize before_size = ctx->stack_frame_size;
        compile_expression(ctx, if_node->condition, instructions);

        // Fake pop the result of the condition to allow the body to compile
        // normaly
        ctx->stack_frame_size -= sizeof(bool);

        Array<Inst> then_instructions;
        array_init(&then_instructions, 2, ctx->arena);
        compile_block(ctx, if_node->then_branch->as_block(),
                      &then_instructions);

        ctx->stack_frame_size += sizeof(bool);

        isize if_false_ip = instructions->size + then_instructions.size + 3;

        core_assert(ctx->stack_frame_size >= (isize)sizeof(bool));
        Inst jmp = inst_jump_if_not(
            mem_ptr_stack_rel(ctx->stack_frame_size - (isize)sizeof(bool)),
            if_false_ip);
        array_push(instructions, jmp);
        pop_stack(ctx, sizeof(bool), instructions);
        array_push_from_slice(instructions, array_to_slice(&then_instructions));

        if (if_node->else_branch != nullptr) {
            Array<Inst> else_instructions;
            array_init(&else_instructions, 2, ctx->arena);

            compile_block(ctx, if_node->else_branch->as_block(),
                          &else_instructions);

            Inst jmp_else = inst_jump(if_false_ip + else_instructions.size + 1);
            array_push(instructions, jmp_else);

            ctx->stack_frame_size += sizeof(bool);
            pop_stack(ctx, sizeof(bool), instructions);

            array_push_from_slice(instructions,
                                  array_to_slice(&else_instructions));
        } else {
            Inst jmp = inst_jump(instructions->size + 2);
            array_push(instructions, jmp);

            ctx->stack_frame_size += sizeof(bool);
            pop_stack(ctx, sizeof(bool), instructions);
        }

        core_assert(ctx->stack_frame_size == before_size);

        break;
    }
    case AstNodeKind::Call: {
        compile_expression(ctx, statement, instructions);
        // Pop the result of the call from the stack as we don't need it
        Type* type = type_set_get_single(statement->type_set);
        pop_stack(ctx, type->size, instructions);
        break;
    }
    case AstNodeKind::Return: {
        AstNodeReturn* ret = statement->as_return();
        if (ret->value) {
            Type* type = type_set_get_single(ret->value->type_set);
            isize before_size = ctx->stack_frame_size;

            compile_expression(ctx, ret->value, instructions);
            MemPtr current_return_ptr =
                ctx->return_ptrs[ctx->return_ptrs.size - 1];

            Inst mov = inst_mov(current_return_ptr,
                                mem_ptr_stack_rel(before_size), type->size);
            array_push(instructions, mov);
        }

        // Here we do not call `pop_stack` as we don't want to change the
        // stack frame size, as we are returning from the function. This
        // would break if there return was in an if statement or something
        // similar, where it may or may not run
        Inst pop_inst = inst_pop_stack(ctx->stack_frame_size);
        array_push(instructions, pop_inst);

        Inst ret_inst = inst_return();
        array_push(instructions, ret_inst);

        break;
    }
    case AstNodeKind::Block: {
        compile_block(ctx, statement->as_block(), instructions);
        break;
    }
    case AstNodeKind::Declaration: {
        // We are declaring a variable on the stack
        AstNodeDeclaration* decl = statement->as_declaration();
        decl->name->ptr = mem_ptr_stack_rel(ctx->stack_frame_size);

        // NOTE(juraj): We don't need to push the stack here, as the
        // result of the expression will be left on the top of the stack
        compile_expression(ctx, decl->value, instructions);

        break;
    }
    case AstNodeKind::Assignment: {
        isize before_size = ctx->stack_frame_size;
        AstNodeAssignment* assign = statement->as_assignment();
        AstNodeIdentifier* name = assign->name->as_identifier();
        MemPtr def_ptr = {};
        switch (name->def->kind) {
        case AstNodeKind::Declaration: {
            AstNodeDeclaration* decl = name->def->as_declaration();
            def_ptr = decl->name->ptr;
            break;
        }
        case AstNodeKind::Parameter: {
            AstNodeParameter* param = name->def->as_parameter();
            def_ptr = param->name->ptr;
            break;
        }
        default: {
            core_assert(false);
            break;
        }
        }

        Type* type = type_set_get_single(assign->name->type_set);

        // Evaluate the expression on the right, the result will be
        // stored on the top of the stack
        compile_expression(ctx, assign->value, instructions);

        // Move the result from the top of the stack to the memory
        // location
        Inst mov = inst_mov(
            def_ptr, mem_ptr_stack_rel(ctx->stack_frame_size - type->size),
            type->size);
        array_push(instructions, mov);

        // Pop the resulting value from the stack
        pop_stack(ctx, type->size, instructions);

        core_assert(ctx->stack_frame_size == before_size);

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
    isize before_size = ctx->stack_frame_size;

    for (isize i = 0; i < block->statements.size; i++) {
        compile_statement(ctx, block->statements[i], instructions);
    }

    isize after_size = ctx->stack_frame_size;
    if (before_size != after_size) {
        core_assert(after_size > before_size);
        pop_stack(ctx, after_size - before_size, instructions);
    }
}

void compile_function(CompilerContext* ctx, AstNodeFunction* function,
                      isize function_offset) {
    Array<Inst> instructions = {};
    array_init(&instructions, 32, ctx->arena);
    function->offset = function_offset;

    isize offset = -CALL_METADATA_SIZE;
    for (isize i = function->parameters.size - 1; i >= 0; i--) {
        AstNodeParameter* param = function->parameters[i];
        Type* type = type_set_get_single(param->type_set);
        offset -= type->size;
        param->name->ptr = mem_ptr_stack_rel(offset);
    }

    FunctionType* function_type =
        type_set_get_single(function->type_set)->as_function();
    Type* return_type = type_set_get_single(function_type->return_type);

    isize return_value_offset = offset - return_type->size;
    array_push(&ctx->return_ptrs, mem_ptr_stack_rel(return_value_offset));
    defer(array_pop(&ctx->return_ptrs));

    ctx->stack_frame_size = 0;

    compile_block(ctx, function->body, &instructions);

    if (instructions.size == 0 ||
        instructions[instructions.size - 1].type != InstType::Return) {
        pop_stack(ctx, ctx->stack_frame_size, &instructions);
        array_push(&instructions, inst_return());
    }

    ctx->functions[function_offset] = array_to_slice(&instructions);
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

    Array<MemPtr> return_ptrs = {};
    array_init(&return_ptrs, 1, arena);

    CompilerContext ctx = {
        .arena = arena,
        .functions = functions,
        .static_data = static_data,
        .function_name_offset_map = function_name_offset,
        .stack_frame_size = 0,
        .return_ptrs = return_ptrs,
    };

    // Do a first pass, where we register all the functions and all the
    // constants. This must be done, as we need to know the function
    // offsets of other functions, when we generate the bytecode for any
    // function. (and we support out of order definitions)
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
            name->ptr = mem_ptr_static_data(offset);
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

        switch (value->kind) {
        case AstNodeKind::Function: {
            isize function_offset = hash_map_must_get(
                &ctx.function_name_offset_map, name->token.source);
            compile_function(&ctx, value->as_function(), function_offset);
            break;
        }
        default: {
            break;
        }
        }
    }

    add_init_function(&ctx);

    CodeUnit code = {
        .static_data = array_to_slice(&ctx.static_data),
        .functions = array_to_slice(&ctx.functions),
    };

    return code;
}
