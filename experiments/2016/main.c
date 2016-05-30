// For strdup
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define SLIM_HASH_IMPLEMENTATION
#include "slim_hash.h"

#include "utils.h"
#include "lexer.h"
#include "parser.h"
#include "reg_alloc.h"


node_p expand_uops(node_p node, uint32_t level, uint32_t flags);
void compile(node_p node, const char* filename);


int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "usage: %s source-file\n", argv[0]);
		return 1;
	}
	
	str_t src = str_fload(argv[1]);
	token_list_p list = lex_str(src, argv[1], stderr);
	
	if (list->error_count > 0) {
		// Just output errors and exit
		for(size_t i = 0; i < list->tokens.len; i++) {
			token_p t = &list->tokens.ptr[i];
			if (t->type == T_ERROR) {
				fprintf(stderr, "%s:%d:%d: %s\n", argv[1], token_line(t), token_col(t), t->str_val.ptr);
				token_print_line(stderr, t, 0);
			}
		}
		
		lex_free(list);
		str_free(&src);
		return 1;
	}
	
	for(size_t i = 0; i < list->tokens.len; i++) {
		token_p t = &list->tokens.ptr[i];
		if (t->type == T_COMMENT || t->type == T_WS) {
			token_print(stdout, t, TP_SOURCE);
		} else if (t->type == T_WSNL || t->type == T_EOF) {
			printf(" ");
			token_print(stdout, t, TP_DUMP);
			printf(" ");
		} else {
			token_print(stdout, t, TP_DUMP);
		}
	}
	printf("\n");
	
	printf("\n");
	node_p tree = parse(list, parse_module);
	printf("\n");
	node_print(tree, stdout);
	
	node_iterate(tree, expand_uops);
	node_print(tree, stdout);
	
	compile(tree, "main.elf");
	
	lex_free(list);
	str_free(&src);
	
	return 0;
}



//
// Normal transformation passes
// 
// For now only transform unordered operations (uops) into binary operator
// trees (op nodes).
//

// Based on http://en.cppreference.com/w/c/language/operator_precedence
// Worth a look because of bitwise and comparison ops: http://wiki.dlang.org/Operator_precedence
typedef enum { LEFT_TO_RIGHT, RIGHT_TO_LEFT } op_assoc_t;
typedef enum {
	OP_MUL, OP_DIV,
	OP_ADD, OP_SUB,
	OP_LT, OP_LE, OP_GT, OP_GE,
	OP_EQ, OP_NEQ,
	OP_ASSIGN
} op_idx_t;
// CAUTION: Operators with the same precedence need the same associativity!
// Otherwise it probably gets complicates... not sure.
struct { char* name; int precedence; op_assoc_t assoc; } operators[] = {
	[OP_MUL]    = { "*",  80, LEFT_TO_RIGHT },
	[OP_DIV]    = { "/",  80, LEFT_TO_RIGHT },
	
	[OP_ADD]    = { "+",  70, LEFT_TO_RIGHT },
	[OP_SUB]    = { "-",  70, LEFT_TO_RIGHT },
	
	[OP_LT]     = { "<",  50, LEFT_TO_RIGHT },
	[OP_LE]     = { "<=", 50, LEFT_TO_RIGHT },
	[OP_GT]     = { ">",  50, LEFT_TO_RIGHT },
	[OP_GE]     = { ">=", 50, LEFT_TO_RIGHT },
	
	[OP_EQ]     = { "==", 40, LEFT_TO_RIGHT },
	[OP_NEQ]    = { "!=", 40, LEFT_TO_RIGHT },
	
	[OP_ASSIGN] = { "=",   0, RIGHT_TO_LEFT },
};

/**
 * For each uops node: We find the strongest binding operator and replace it and
 * the nodes it binds to with an op node. This is repeated until no operators
 * are left to replace. The uops node should just have one op child at the end.
 */
node_p expand_uops(node_p node, uint32_t level, uint32_t flags) {
	// We only want uops nodes with children in them (there should be no uops
	// nodes without children).
	if ( !( (flags & NI_PRE) && node->type == NT_UOPS ) )
		return node;
	
	node_list_p list = &node->uops.list;
	
	while (list->len > 1) {
		// Find strongest operator (operator with highest precedence)
		int strongest_op_idx = -1;
		int strongest_op_prec = 0;
		int strongest_op_node_idx = -1;
		for(int node_idx = 1; node_idx < (int)list->len; node_idx += 2) {
			node_p op_slot = list->ptr[node_idx];
			if (op_slot->type != NT_ID) {
				fprintf(stderr, "expand_uops(): got non NT_ID in uops op slot!\n");
				abort();
			}
			
			// Find operator of the current op_slot node based on the IDs name
			ssize_t op_idx = -1;
			for(size_t i = 0; i < sizeof(operators) / sizeof(operators[0]); i++) {
				if ( strncmp(operators[i].name, op_slot->id.name.ptr, op_slot->id.name.len) == 0 && (int)strlen(operators[i].name) == op_slot->id.name.len ) {
					op_idx = i;
					break;
				}
			}
			
			if (op_idx == -1) {
				fprintf(stderr, "expand_uops(): unknown operator: %.*s!",
					op_slot->id.name.len, op_slot->id.name.ptr);
				abort();
			}
			
			printf("expand_uops(): got op %.*s (idx %zd) at node idx %d\n",
				op_slot->id.name.len, op_slot->id.name.ptr, op_idx, node_idx
			);
			
			// When several ops have the highest (same) precedence:
			// For LEFT_TO_RIGHT operators we pick the first op
			// For RIGHT_TO_LEFT operators we pick the last op
			if (
				(operators[op_idx].assoc == LEFT_TO_RIGHT && operators[op_idx].precedence >  strongest_op_prec) ||
				(operators[op_idx].assoc == RIGHT_TO_LEFT && operators[op_idx].precedence >= strongest_op_prec)
			) {
				strongest_op_prec = operators[op_idx].precedence;
				strongest_op_idx = op_idx;
				strongest_op_node_idx = node_idx;
			}
		}
		
		if (strongest_op_idx == -1) {
			fprintf(stderr, "expand_uops(): found no op in uops node!");
			abort();
		}
		
		printf("expand_uops(): op %s got highest precedence: %d\n",
			operators[strongest_op_idx].name, strongest_op_prec
		);
		
		
		// Take the nodes left and right from the op_slot node and create a new op
		// node out of them. Then replace these nodes in the list with the new op
		// node.
		node_p op_node = node_alloc(NT_OP);
		op_node->parent = node;
		op_node->op.idx = strongest_op_idx;
		node_set(op_node, &op_node->op.a, list->ptr[strongest_op_node_idx - 1]);
		node_set(op_node, &op_node->op.b, list->ptr[strongest_op_node_idx + 1]);
		
		node_list_replace_n1(list, strongest_op_node_idx - 1, 3, op_node);
		
		node_print(node, stderr);
	}
	
	// By now the uops node only contains one op node child. Return that so the
	// iteration function replaces this uops node with the returned op node.
	return node->uops.list.ptr[0];
}



//
// Scope (support stuff for compilation of variables)
//

typedef struct {
	size_t stack_offset;
} scope_binding_t, *scope_binding_p;

SH_GEN_DECL(scope_dict, const char*, scope_binding_t);

typedef struct scope_s scope_t, *scope_p;
struct scope_s {
	scope_p parent;
	size_t allocated_stack_space;
	scope_dict_t bindings;
};

SH_GEN_DICT_DEF(scope_dict, const char*, scope_binding_t);

void scope_new(scope_p scope, scope_p parent) {
	scope->parent = parent;
	scope->allocated_stack_space = (parent != NULL) ? parent->allocated_stack_space : 0;
	scope_dict_new(&scope->bindings);
}

void scope_destroy(scope_p scope) {
	scope_dict_destroy(&scope->bindings);
}

size_t scope_put(scope_p scope, const char* name, size_t size) {
	scope_binding_p binding = scope_dict_put_ptr(&scope->bindings, name);
	
	binding->stack_offset = scope->allocated_stack_space;
	scope->allocated_stack_space += size;
	
	return binding->stack_offset;
}

size_t scope_get(scope_p scope, const char* name) {
	for(scope_p current_scope = scope; current_scope != NULL; current_scope = current_scope->parent) {
		scope_binding_p binding = scope_dict_get_ptr(&current_scope->bindings, name);
		if (binding != NULL)
			return binding->stack_offset;
	}
	return -1;
}



//
// Compiler stuff (tree to asm)
//

typedef struct {
	asm_p as;
	ra_p ra;
	scope_p scope;
} compiler_ctx_t, *compiler_ctx_p;

raa_t compile_node(node_p node, compiler_ctx_p ctx, int8_t requested_result_register);
raa_t compile_module(node_p node, compiler_ctx_p ctx);
raa_t compile_scope(node_p node, compiler_ctx_p ctx);
raa_t compile_syscall(node_p node, compiler_ctx_p ctx);
raa_t compile_var(node_p node, compiler_ctx_p ctx);
raa_t compile_if(node_p node, compiler_ctx_p ctx);
raa_t compile_while(node_p node, compiler_ctx_p ctx);
raa_t compile_op(node_p node, compiler_ctx_p ctx, int8_t req_reg);
raa_t compile_intl(node_p node, compiler_ctx_p ctx, int8_t req_reg);
raa_t compile_strl(node_p node, compiler_ctx_p ctx, int8_t req_reg);
raa_t compile_id(node_p node, compiler_ctx_p ctx, int8_t req_reg);



void compile(node_p node, const char* filename) {
	compiler_ctx_t ctx = (compiler_ctx_t){
		.as = &(asm_t){ 0 },
		.ra = &(ra_t){ 0 },
		.scope = NULL
	};
	as_new(ctx.as);
	ra_new(ctx.ra);
	
	raa_t a = compile_node(node, &ctx, -1);
	ra_free_reg(ctx.ra, ctx.as, a);
	as_save_elf(ctx.as, filename);
	
	ra_destroy(ctx.ra);
	as_destroy(ctx.as);
}

raa_t compile_node(node_p node, compiler_ctx_p ctx, int8_t requested_result_register) {
	switch(node->type) {
		case NT_MODULE:
			return compile_module(node, ctx);
		case NT_SCOPE:
			return compile_scope(node, ctx);
		case NT_SYSCALL:
			return compile_syscall(node, ctx);
		case NT_VAR:
			return compile_var(node, ctx);
		case NT_IF:
			return compile_if(node, ctx);
		case NT_WHILE:
			return compile_while(node, ctx);
		case NT_INTL:
			return compile_intl(node, ctx, requested_result_register);
		case NT_STRL:
			return compile_strl(node, ctx, requested_result_register);
		case NT_OP:
			return compile_op(node, ctx, requested_result_register);
		case NT_ID:
			return compile_id(node, ctx, requested_result_register);
		
		case NT_UOPS:
			fprintf(stderr, "compile_node(): uops nodes have to be reordered to op nodes before compilation!\n");
			abort();
			
		case NT_UNARY_OP:
			fprintf(stderr, "compile_node(): TODO\n");
			abort();
	}
	
	return (raa_t){ -1, -1 };
}

raa_t compile_module(node_p node, compiler_ctx_p ctx) {
	scope_t scope;
	scope_new(&scope, ctx->scope);
	ctx->scope = &scope;
	
	for(size_t i = 0; i < node->module.stmts.len; i++) {
		raa_t allocation = compile_node(node->module.stmts.ptr[i], ctx, -1);
		// Free allocations in case the statement is an expr (syscall and var
		// don't return an allocated register)
		ra_free_reg(ctx->ra, ctx->as, allocation);
	}
	
	ctx->scope = scope.parent;
	scope_destroy(&scope);
	
	return (raa_t){ -1, -1 };
}

raa_t compile_scope(node_p node, compiler_ctx_p ctx) {
	// compile_module() does the exact same for now...
	return compile_module(node, ctx);
}

raa_t compile_syscall(node_p node, compiler_ctx_p ctx) {
	/*
	Input:
		RAX ← syscall_no
		RDI
		RSI
		RDX
		RCX
		R8
		R9
	Scratch
		All input regs
		R10
		R11
	*/
	
	int8_t arg_regs[7] = { RAX.reg, RDI.reg, RSI.reg, RDX.reg, RCX.reg, R8.reg, R9.reg };
	raa_t arg_allocs[7];
	
	// Need at least one argument (the syscall number)
	if (node->syscall.args.len < 1) {
		fprintf(stderr, "compile_syscall(): need at least 1 arg, got %zu\n", node->syscall.args.len);
		abort();
	}
	
	// Compile args
	for(size_t i = 0; i < node->syscall.args.len; i++)
		arg_allocs[i] = compile_node(node->syscall.args.ptr[i], ctx, arg_regs[i]);
	
	// Allocate scratch registers
	// TODO: Also allocate unused args as scratch regs
	raa_t a1 = ra_alloc_reg(ctx->ra, ctx->as, R10.reg);
	raa_t a2 = ra_alloc_reg(ctx->ra, ctx->as, R11.reg);
	
	as_syscall(ctx->as);
	
	// Free scratch registers
	ra_free_reg(ctx->ra, ctx->as, a2);
	ra_free_reg(ctx->ra, ctx->as, a1);
	
	// Free argument registers
	for(size_t i = 0; i < node->syscall.args.len; i++)
		ra_free_reg(ctx->ra, ctx->as, arg_allocs[i]);
	
	return (raa_t){ -1, -1 };
}

raa_t compile_var(node_p node, compiler_ctx_p ctx) {
	char* terminated_name = strndup(node->var.name.ptr, node->var.name.len);
	
	size_t stack_offset = scope_put(ctx->scope, terminated_name, 8);
	if (node->var.value != NULL) {
		raa_t a = compile_node(node->var.value, ctx, -1);
		as_mov(ctx->as, memrd(RSP, stack_offset), reg(a.reg_index));
		ra_free_reg(ctx->ra, ctx->as, a);
	}
	
	free(terminated_name);
	return (raa_t){ -1, -1 };
}

raa_t compile_if(node_p node, compiler_ctx_p ctx) {
	raa_t a = compile_node(node->if_stmt.cond, ctx, -1);
	as_cmp(ctx->as, reg(a.reg_index), imm(0));
	// freeing might add a MOV here to restore the previous value of a register
	// but a MOV doesn't effect the Flags. So it's safe to use here.
	ra_free_reg(ctx->ra, ctx->as, a);
	// We jump to the false case if the condition is equal to 0 (false)
	asm_jump_slot_t to_false_case = as_jmp_cc(ctx->as, CC_EQUAL, 0);
	
	a = compile_node(node->if_stmt.true_case, ctx, -1);
	ra_free_reg(ctx->ra, ctx->as, a);
	asm_jump_slot_t to_end = as_jmp(ctx->as, reld(0));
	
	as_mark_jmp_slot_target(ctx->as, to_false_case);
	a = compile_node(node->if_stmt.false_case, ctx, -1);
	ra_free_reg(ctx->ra, ctx->as, a);
	
	as_mark_jmp_slot_target(ctx->as, to_end);
	return (raa_t){ -1, -1 };
}

raa_t compile_while(node_p node, compiler_ctx_p ctx) {
	raa_t a;
	
	// Compile condition
	size_t cond_target = as_target(ctx->as);
	a = compile_node(node->while_stmt.cond, ctx, -1);
	as_cmp(ctx->as, reg(a.reg_index), imm(0));
	// freeing might add a MOV here to restore the previous value of a register
	// but a MOV doesn't effect the Flags. So it's safe to use here.
	ra_free_reg(ctx->ra, ctx->as, a);
	// We jump to the end of the loop if the condition is equal to 0 (false)
	asm_jump_slot_t to_end = as_jmp_cc(ctx->as, CC_EQUAL, 0);
	
	// Compile body, then jump back to condition
	a = compile_node(node->while_stmt.body, ctx, -1);
	ra_free_reg(ctx->ra, ctx->as, a);
	asm_jump_slot_t to_cond = as_jmp(ctx->as, reld(0));
	as_set_jmp_slot_target(ctx->as, to_cond, cond_target);
	
	as_mark_jmp_slot_target(ctx->as, to_end);
	return (raa_t){ -1, -1 };
}


raa_t compile_op(node_p node, compiler_ctx_p ctx, int8_t req_reg) {
	size_t op = node->op.idx;
	node_p a = node->op.a, b = node->op.b;
	
	switch(op) {
		case OP_ADD: case OP_SUB:
		{
			raa_t a1 = compile_node(a, ctx, req_reg);
			raa_t a2 = compile_node(b, ctx, -1);
			
			switch(op) {
				case OP_ADD:  as_add(ctx->as, reg(a1.reg_index), reg(a2.reg_index));  break;
				case OP_SUB:  as_sub(ctx->as, reg(a1.reg_index), reg(a2.reg_index));  break;
			}
			
			ra_free_reg(ctx->ra, ctx->as, a2);
			return a1;
		}
		
		case OP_MUL: case OP_DIV:
		{
			// reserve RDX since MUL and DIV overwrite it
			raa_t a1 = ra_alloc_reg(ctx->ra, ctx->as, RDX.reg);
			raa_t a2 = compile_node(a, ctx, RAX.reg);
			raa_t a3 = compile_node(b, ctx, -1);
			
			switch(op) {
				case OP_MUL:  as_mul(ctx->as, reg(a3.reg_index));  break;
				case OP_DIV:  as_div(ctx->as, reg(a3.reg_index));  break;
			}
			
			ra_free_reg(ctx->ra, ctx->as, a1);
			ra_free_reg(ctx->ra, ctx->as, a3);
			
			if (req_reg != RAX.reg && req_reg != -1) {
				raa_t a4 = ra_alloc_reg(ctx->ra, ctx->as, req_reg);
				as_mov(ctx->as, reg(req_reg), RAX);
				ra_free_reg(ctx->ra, ctx->as, a2);
				return a4;
			} else {
				return a2;
			}
		}
		
		case OP_LT: case OP_LE: case OP_GT: case OP_GE:
		case OP_EQ: case OP_NEQ:
		{
			uint8_t condition_code;
			switch(op) {
				case OP_LT:  condition_code = CC_LESS;             break;
				case OP_LE:  condition_code = CC_LESS_OR_EQUAL;    break;
				case OP_GT:  condition_code = CC_GREATER;          break;
				case OP_GE:  condition_code = CC_GREATER_OR_EQUAL; break;
				case OP_EQ:  condition_code = CC_EQUAL;            break;
				case OP_NEQ: condition_code = CC_NOT_EQUAL;        break;
				default:     abort();
			}
			
			raa_t a1 = compile_node(a, ctx, req_reg);
			raa_t a2 = compile_node(b, ctx, -1);
			as_cmp(ctx->as, reg(a1.reg_index), reg(a2.reg_index));
			ra_free_reg(ctx->ra, ctx->as, a2);
			
			as_set_cc(ctx->as, condition_code, reg(a1.reg_index));
			return a1;
		}
		
		case OP_ASSIGN:
		{
			if (a->type != NT_ID) {
				fprintf(stderr, "compile_op(): right side of assigment has to be an ID for now!\n");
				abort();
			}
			
			char* terminated_name = strndup(a->id.name.ptr, a->id.name.len);
			
			size_t stack_offset = scope_get(ctx->scope, terminated_name);
			raa_t alloc = compile_node(node->var.value, ctx, -1);
			as_mov(ctx->as, memrd(RSP, stack_offset), reg(alloc.reg_index));
			// keep value as result value of this operation
			//ra_free_reg(ctx->ra, ctx->as, alloc);
			
			free(terminated_name);
			return alloc;
		}
	}
	
	fprintf(stderr, "compile_op(): unknown op idx: %zu!\n", op);
	abort();
	return (raa_t){ -1, -1 };
}

raa_t compile_intl(node_p node, compiler_ctx_p ctx, int8_t req_reg) {
	raa_t a = ra_alloc_reg(ctx->ra, ctx->as, req_reg);
	as_mov(ctx->as, reg(a.reg_index), imm(node->intl.value));
	return a;
}

raa_t compile_strl(node_p node, compiler_ctx_p ctx, int8_t req_reg) {
	size_t str_vaddr = as_data(ctx->as, node->strl.value.ptr, node->strl.value.len);
	raa_t a = ra_alloc_reg(ctx->ra, ctx->as, req_reg);
	as_mov(ctx->as, reg(a.reg_index), imm(str_vaddr));
	return a;
}

raa_t compile_id(node_p node, compiler_ctx_p ctx, int8_t req_reg) {
	char* terminated_name = strndup(node->var.name.ptr, node->var.name.len);
	int32_t stack_offset = scope_get(ctx->scope, terminated_name);
	free(terminated_name);
	
	raa_t a = ra_alloc_reg(ctx->ra, ctx->as, req_reg);
	as_mov(ctx->as, reg(a.reg_index), memrd(RSP, stack_offset));
	
	return a;
}