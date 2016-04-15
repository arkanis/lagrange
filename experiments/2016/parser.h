#pragma once
#include "lexer.h"

typedef struct {
	int type;
} tree_t, *tree_p;

tree_p parse_module(tokenized_file_p file);
void parser_free_tree(tree_p tree);