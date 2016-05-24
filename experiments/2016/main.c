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
	token_list_p list = lex_str(src.ptr, src.len, argv[1], stderr);
	
	if (list->error_count > 0) {
		// Just output errors and exit
		for(size_t i = 0; i < list->tokens_len; i++) {
			token_p t = &list->tokens_ptr[i];
			if (t->type == T_ERROR) {
				fprintf(stderr, "%s:%d:%d: %s\n", argv[1], token_line(t), token_col(t), t->str_val);
				token_print_line(stderr, t, 0);
			}
		}
		
		lex_free(list);
		str_free(&src);
		return 1;
	}
	
	for(size_t i = 0; i < list->tokens_len; i++) {
		token_p t = &list->tokens_ptr[i];
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
// For now only transform unordered operations (uops_t) into binary operator
// trees (op_t) nodes.
//

node_p expand_uops(node_p node, uint32_t level, uint32_t flags) {
	// We only want uops nodes with children in them (there should be no uops
	// nodes without children).
	if ( !( (flags & NI_PRE) && node->type == NT_UOPS ) )
		return node;
	
	node_list_p list = &node->uops.list;
	
	// Find weakest operator
	int weakest_op_idx = -1;
	int weakest_op_weight = 10000;
	char weakest_op = '\0';
	for(int op_idx = 1; op_idx < (int)list->len; op_idx += 2) {
		
		int weight = 0;
		char op = '\0';
		
		if (list->ptr[op_idx]->type == NT_ID) {
			node_p id = list->ptr[op_idx];
			switch(id->id.name.ptr[0]) {
				case '+': weight = 10; op = '+'; break;
				case '-': weight = 10; op = '-'; break;
				case '*': weight = 20; op = '*'; break;
				case '/': weight = 20; op = '/'; break;
				case '=': weight = 0;  op = '='; break;
			}
			printf("got op %.*s, weight: %d\n", (int)id->id.name.len, id->id.name.ptr, weight);
			
			if (weight < weakest_op_weight) {
				weakest_op_weight = weight;
				weakest_op_idx = op_idx;
				weakest_op = op;
			}
		} else {
			fprintf(stderr, "expand_uops(): got non NT_ID in uops op slot!");
			abort();
		}
	}
	
	if (weakest_op_idx == -1) {
		fprintf(stderr, "expand_uops(): found no op in uops!");
		abort();
	}
	
	printf("got weakest op at %d, weight: %d\n", weakest_op_idx, weakest_op_weight);
	
	// Split uops node into an op node with two uops children
	node_p op_node = node_alloc(NT_OP);
	op_node->parent = node->parent;
	op_node->op.op = weakest_op;
	
	if (weakest_op_idx == 1) {
		op_node->op.a = list->ptr[0];
	} else {
		node_p first = node_alloc(NT_UOPS);
		first->parent = op_node;
		op_node->op.a = first;
		
		for(int i = 0; i < weakest_op_idx; i++)
			buf_append(&first->uops.list, list->ptr[i]);
	}
	
	if (weakest_op_idx == (int)list->len - 2) {
		op_node->op.b = list->ptr[list->len - 1];
	} else {
		node_p rest = node_alloc(NT_UOPS);
		rest->parent = op_node;
		op_node->op.b = rest;
		
		for(int i = weakest_op_idx + 1; i < (int)list->len; i++)
			buf_append(&rest->uops.list, list->ptr[i]);
	}
	
	return op_node;
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
	scope->allocated_stack_space = 0;
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
	scope_binding_p binding = scope_dict_get_ptr(&scope->bindings, name);
	if (binding == NULL)
		return -1;
	return binding->stack_offset;
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
raa_t compile_syscall(node_p node, compiler_ctx_p ctx);
raa_t compile_var(node_p node, compiler_ctx_p ctx);
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
		case NT_SYSCALL:
			return compile_syscall(node, ctx);
		case NT_VAR:
			return compile_var(node, ctx);
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
			
		/*
		case NT_ID:
			fprintf(stderr, "compile_node(): TODO\n");
			abort();
		*/
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


raa_t compile_op(node_p node, compiler_ctx_p ctx, int8_t req_reg) {
	char op = node->op.op;
	node_p a = node->op.a, b = node->op.b;
	
	if (op == '+' || op == '-') {
		raa_t a1 = compile_node(a, ctx, req_reg);
		raa_t a2 = compile_node(b, ctx, -1);
		
		switch(op) {
			case '+':  as_add(ctx->as, reg(a1.reg_index), reg(a2.reg_index));  break;
			case '-':  as_sub(ctx->as, reg(a1.reg_index), reg(a2.reg_index));  break;
		}
		
		ra_free_reg(ctx->ra, ctx->as, a2);
		return a1;
	} else if (op == '*' || op == '/') {
		// reserve RDX since MUL and DIV overwrite it
		raa_t a1 = ra_alloc_reg(ctx->ra, ctx->as, RDX.reg);
		raa_t a2 = compile_node(a, ctx, RAX.reg);
		raa_t a3 = compile_node(b, ctx, -1);
		
		switch(op) {
			case '*':  as_mul(ctx->as, reg(a3.reg_index));  break;
			case '/':  as_div(ctx->as, reg(a3.reg_index));  break;
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
	} else if (op == '=') {
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
	
	fprintf(stderr, "compile_op(): unknown operation: %c!\n", op);
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