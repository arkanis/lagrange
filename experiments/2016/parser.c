#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "parser.h"


//
// Parser state and utility stuff
//

typedef struct {
	token_p tokens;
	size_t token_count, pos;
} parser_t, *parser_p;


token_p peek(parser_p parser) {
	if (parser->pos + 1 < parser->token_count)
		return &parser->tokens[parser->pos+1];
	
	fprintf(stderr, "peeked beyond EOF!\n");
	return NULL;
}

token_type_t peek_type(parser_p parser) {
	return peek(parser)->type;
}

token_t consume(parser_p parser) {
	if (parser->pos < parser->token_count)
		return parser->tokens[parser->pos++];
	
	fprintf(stderr, "consumed token beyond EOF!\n");
	abort();
}

token_t consume_type(parser_p parser, token_type_t type) {
	token_t t = consume(parser);
	if (t.type == type)
		return t;
	
	fprintf(stderr, "consumed wrong token! expected type %d, got %d\n", type, t.type);
	abort();
}



tree_p parse_func_def(parser_p parser);
tree_p parse_var_def(parser_p parser);
bool is_stmt_start(token_type_t type);
tree_p parse_stmt(parser_p parser);
bool is_expr_start(token_type_t type);
tree_p parse_expr(parser_p parser);


//
// Definitions
//

tree_p parse_module(token_p tokens, size_t token_count) {
	parser_t parser_storage = { tokens, token_count, 0 };
	parser_p parser = &parser_storage;
	
	printf("module:\n");
	while (true) {
		if ( peek_type(parser) == T_FUNC ) {
			parse_func_def(parser);
		} else if ( is_expr_start(peek_type(parser)) ) {
			parse_var_def(parser);
		} else {
			break;
		}
	}
	consume_type(parser, T_EOF);
	
	return NULL;
}

tree_p parse_func_def(parser_p parser) {
	printf("  func");
	consume_type(parser, T_FUNC);
	token_t id = consume_type(parser, T_ID);
	printf(" id %.*s:\n", (int)id.length, id.str_val);
	consume_type(parser, T_CBO);
	
	while ( is_stmt_start(peek_type(parser)) )
		parse_stmt(parser);
	
	consume_type(parser, T_CBC);
	
	return NULL;
}

tree_p parse_var_def(parser_p parser) {
	parser = parser;
	abort();
}


//
// Statements
//

bool is_stmt_start(token_type_t type) {
	switch(type) {
		case T_RET:
			return true;
		default:
			return is_expr_start(type);
	}
}

tree_p parse_stmt(parser_p parser) {
	printf("    stmt:\n");
	consume_type(parser, T_RET);
	parse_expr(parser);
	consume_type(parser, T_EOS);
	return NULL;
}


//
// Expressions
//

bool is_expr_start(token_type_t type) {
	switch(type) {
		case T_ID:
		case T_RBO:
		case T_INT:
		case T_STR:
			return true;
		default:
			return false;
	}
}

tree_p parse_expr(parser_p parser) {
	token_t t = consume(parser);
	switch(t.type) {
		case T_ID:
			printf("      expr id: %.*s\n", (int)t.length, t.str_val);
			return NULL;
		case T_INT:
			printf("      expr int: %d\n", t.int_val);
			return NULL;
		case T_STR:
			printf("      expr str: \"%.*s\"\n", (int)t.length, t.str_val);
			return NULL;
		default:
			abort();
			return NULL;
	}
}