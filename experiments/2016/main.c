#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "lexer.h"
#include "parser.h"

void* sgl_fload(const char* filename, size_t* size);
node_p expand_uops(node_p node, uint32_t level, uint32_t flags);
node_p demo_pass(node_p node, uint32_t level, uint32_t flags);

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
		} else if (t->type == T_WS_EOS || t->type == T_EOF) {
			printf(" ");
			token_print(stdout, t, TP_DUMP);
			printf(" ");
		} else {
			token_print(stdout, t, TP_DUMP);
		}
	}
	printf("\n");
	
	printf("\n");
	node_p tree = parse(list, parse_stmt);
	printf("\n");
	node_print(tree, stdout, 0);
	
	node_iterate(tree, 0, demo_pass);
	node_iterate(tree, 0, expand_uops);
	node_print(tree, stdout, 0);
	
	lex_free(list);
	free(src_str);
	
	return 0;
}

node_p demo_pass(node_p node, uint32_t level, uint32_t flags) {
	if (flags & NI_PRE)
		printf("%*sPRE  %p\n", (int)level*2, "", node);
	else if (flags & NI_POST)
		printf("%*sPOST %p\n", (int)level*2, "", node);
	else if (flags & NI_LEAF)
		printf("%*sLEAF %p\n", (int)level*2, "", node);
	return node;
}

node_p expand_uops(node_p node, uint32_t level, uint32_t flags) {
	// We only want uops nodes with children in them (there should be no uops
	// nodes without children).
	if ( !( (flags & NI_PRE) && node->spec == uops_spec ) )
		return node;
	
	uops_p uops = (uops_p)node;
	
	// Find weakest operator
	int weakest_op_idx = -1;
	int weakest_op_weight = 10000;
	char weakest_op = '\0';
	for(int op_idx = 1; op_idx < (int)uops->list.len; op_idx += 2) {
		
		int weight = 0;
		char op = '\0';
		
		if (uops->list.ptr[op_idx]->spec == sym_spec) {
			sym_p sym = (sym_p)uops->list.ptr[op_idx];
			switch(sym->name[0]) {
				case '+': weight = 10; op = '+'; break;
				case '-': weight = 10; op = '-'; break;
				case '*': weight = 20; op = '*'; break;
				case '/': weight = 20; op = '/'; break;
			}
			printf("got op %.*s, weight: %d\n", (int)sym->size, sym->name, weight);
			
			if (weight < weakest_op_weight) {
				weakest_op_weight = weight;
				weakest_op_idx = op_idx;
				weakest_op = op;
			}
		} else {
			fprintf(stderr, "expand_uops(): got non symbol in uops op slot!");
			abort();
		}
	}
	
	if (weakest_op_idx == -1) {
		fprintf(stderr, "expand_uops(): found no op in uops!");
		abort();
	}
	
	printf("got weakest op at %d, weight: %d\n", weakest_op_idx, weakest_op_weight);
	
	// Split uops node into an op node with two uops children
	op_p op_node = node_alloc(op, uops->node.parent);
	op_node->op = weakest_op;
	
	if (weakest_op_idx == 1) {
		op_node->a = uops->list.ptr[0];
	} else {
		uops_p first = node_alloc(uops, op_node);
		for(int i = 0; i < weakest_op_idx; i++)
			buf_append(first->list, uops->list.ptr[i]);
		op_node->a = (node_p)first;
	}
	
	if (weakest_op_idx == (int)uops->list.len - 2) {
		op_node->b = uops->list.ptr[uops->list.len - 1];
	} else {
		uops_p rest = node_alloc(uops, op_node);
		for(int i = weakest_op_idx + 1; i < (int)uops->list.len; i++)
			buf_append(rest->list, uops->list.ptr[i]);
		op_node->b = (node_p)rest;
	}
	
	return (node_p)op_node;
	
}

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
