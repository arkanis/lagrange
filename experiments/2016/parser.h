#pragma once
#include "lexer.h"

typedef struct {
	int type;
} tree_t, *tree_p;

tree_p parse_module(token_p tokens, size_t token_count);
void parser_free_tree(tree_p tree);