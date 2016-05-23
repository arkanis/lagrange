#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "lexer.h"
#include "parser.h"
#include "reg_alloc.h"


void* sgl_fload(const char* filename, size_t* size);
node_p expand_uops(node_p node, uint32_t level, uint32_t flags);
void compile(node_p node, const char* filename);


int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "usage: %s source-file\n", argv[0]);
		return 1;
	}
	
	size_t src_len = 0;
	char* src_str = sgl_fload(argv[1], &src_len);
	token_list_p list = lex_str(src_str, src_len, argv[1], stderr);
	
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
		free(src_str);
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
	free(src_str);
	
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
// Compiler stuff (tree to asm)
//

raa_t compile_node(node_p node, asm_p assembler, ra_p register_allocator, int8_t requested_result_register);
raa_t compile_module(node_p node, asm_p as, ra_p ra);
raa_t compile_syscall(node_p node, asm_p as, ra_p ra);
raa_t compile_op(node_p node, asm_p as, ra_p ra, int8_t req_reg);
raa_t compile_intl(node_p node, asm_p as, ra_p ra, int8_t req_reg);

void compile(node_p node, const char* filename) {
	asm_p as = &(asm_t){ 0 };
	ra_p ra = &(ra_t){ 0 };
	as_new(as);
	ra_new(ra);
	
	raa_t alloc = compile_node(node, as, ra, -1);
	ra_free_reg(ra, as, alloc);
	as_save_elf(as, filename);
	
	ra_destroy(ra);
	as_destroy(as);
}

raa_t compile_node(node_p node, asm_p assembler, ra_p register_allocator, int8_t requested_result_register) {
	switch(node->type) {
		case NT_MODULE:
			return compile_module(node, assembler, register_allocator);
		case NT_SYSCALL:
			return compile_syscall(node, assembler, register_allocator);
		case NT_INTL:
			return compile_intl(node, assembler, register_allocator, requested_result_register);
		case NT_OP:
			return compile_op(node, assembler, register_allocator, requested_result_register);
		
		case NT_UOPS:
			fprintf(stderr, "compile_node(): uops nodes have to be reordered to op nodes before compilation!\n");
			abort();
			
		case NT_ID:
		case NT_STRL:
			fprintf(stderr, "compile_node(): TODO\n");
			abort();
	}
	
	return (raa_t){ -1, -1 };
}

raa_t compile_module(node_p node, asm_p as, ra_p ra) {
	for(size_t i = 0; i < node->module.stmts.len; i++)
		compile_node(node->module.stmts.ptr[i], as, ra, -1);
	return (raa_t){ -1, -1 };
}

raa_t compile_syscall(node_p node, asm_p as, ra_p ra) {
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
	
	// RAX ← syscall number
	raa_t a1 = ra_alloc_reg(ra, as, RAX.reg);
	as_mov(as, RAX, imm(60));
	
	// RDI ← status code
	raa_t a2 = compile_node(node->syscall.expr, as, ra, RDI.reg);
	
	as_syscall(as);
	
	ra_free_reg(ra, as, a2);
	ra_free_reg(ra, as, a1);
	
	return (raa_t){ -1, -1 };
}

raa_t compile_op(node_p node, asm_p as, ra_p ra, int8_t req_reg) {
	char op = node->op.op;
	node_p a = node->op.a, b = node->op.b;
	
	if (op == '+' || op == '-') {
		raa_t a1 = compile_node(a, as, ra, req_reg);
		raa_t a2 = compile_node(b, as, ra, -1);
		
		switch(op) {
			case '+':  as_add(as, reg(a1.reg_index), reg(a2.reg_index));  break;
			case '-':  as_sub(as, reg(a1.reg_index), reg(a2.reg_index));  break;
		}
		
		ra_free_reg(ra, as, a2);
		return a1;
	} else if (op == '*' || op == '/') {
		// reserve RDX since MUL and DIV overwrite it
		raa_t a1 = ra_alloc_reg(ra, as, RDX.reg);
		raa_t a2 = compile_node(a, as, ra, RAX.reg);
		raa_t a3 = compile_node(b, as, ra, -1);
		
		switch(op) {
			case '*':  as_mul(as, reg(a3.reg_index));  break;
			case '/':  as_div(as, reg(a3.reg_index));  break;
		}
		
		ra_free_reg(ra, as, a1);
		ra_free_reg(ra, as, a3);
		
		if (req_reg != RAX.reg && req_reg != -1) {
			raa_t a4 = ra_alloc_reg(ra, as, req_reg);
			as_mov(as, reg(req_reg), RAX);
			ra_free_reg(ra, as, a2);
			return a4;
		} else {
			return a2;
		}
	}
	
	fprintf(stderr, "compile_op(): unknown operation: %c!\n", op);
	abort();
	return (raa_t){ -1, -1 };
}

raa_t compile_intl(node_p node, asm_p as, ra_p ra, int8_t req_reg) {
	raa_t a = ra_alloc_reg(ra, as, req_reg);
	as_mov(as, reg(a.reg_index), imm(node->intl.value));
	return a;
}


//
// Utility stuff
//

void* sgl_fload(const char* filename, size_t* size) {
	long filesize = 0;
	char* data = NULL;
	int error = -1;
	
	FILE* f = fopen(filename, "rb");
	if (f == NULL)
		return NULL;
	
	if ( fseek(f, 0, SEEK_END)              == -1       ) goto fail;
	if ( (filesize = ftell(f))              == -1       ) goto fail;
	if ( fseek(f, 0, SEEK_SET)              == -1       ) goto fail;
	if ( (data = malloc(filesize + 1))      == NULL     ) goto fail;
	// TODO: proper error detection for fread and get proper error code with ferror
	if ( (long)fread(data, 1, filesize, f)  != filesize ) goto free_and_fail;
	fclose(f);
	
	data[filesize] = '\0';
	if (size)
		*size = filesize;
	return (void*)data;
	
	free_and_fail:
		error = errno;
		free(data);
	
	fail:
		if (error == -1)
			error = errno;
		fclose(f);
	
	errno = error;
	return NULL;
}
