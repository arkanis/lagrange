#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "parser.h"


//
// Parser state and utility stuff
//

typedef struct {
	tokenized_file_p file;
	size_t pos;
} parser_t, *parser_p;


int next_filtered_token_at(parser_p parser, size_t pos, bool ignore_ws_eos) {
	size_t offset = 0;
	while (pos + offset < parser->file->token_count) {
		token_type_t type = parser->file->tokens[pos + offset].type;
		// Return the offset if we found a filtered token there
		if ( !( type == T_WS || type == T_COMMENT || (ignore_ws_eos && type == T_WS_EOS) ) )
			return offset;
		offset++;
	}
	
	// We're either beyond the last token or found no filtered token beyond pos
	return -1;
}

#define consume(parser)          consume_impl((parser), true,  __FUNCTION__, __LINE__)
#define consume_with_eos(parser) consume_impl((parser), false, __FUNCTION__, __LINE__)
token_p consume_impl(parser_p parser, bool ignore_ws_eos, const char* caller, int line) {
	int offset = next_filtered_token_at(parser, parser->pos, ignore_ws_eos);
	// Return NULL if there are no more filtered tokens
	if (offset == -1)
		return NULL;
	
	parser->pos += offset;
	token_p current_token = &parser->file->tokens[parser->pos];
	parser->pos++;
	
	char token_desc[128];
	token_dump(current_token, token_desc, sizeof(token_desc));
	printf("%s:%d consume %s\n", caller, line, token_desc);
	token_print_line(current_token);
	
	return current_token;
}

#define peek(parser)          peek_impl((parser), true,  __FUNCTION__, __LINE__)
#define peek_with_eos(parser) peek_impl((parser), false, __FUNCTION__, __LINE__)
token_p peek_impl(parser_p parser, bool ignore_ws_eos, const char* caller, int line) {
	int offset = next_filtered_token_at(parser, parser->pos, ignore_ws_eos);
	// Return NULL if there are no more filtered tokens
	if (offset == -1)
		return NULL;
	
	token_p current_token = &parser->file->tokens[parser->pos + offset];
	
	char token_desc[128];
	token_dump(current_token, token_desc, sizeof(token_desc));
	printf("%s:%d peek %s\n", caller, line, token_desc);
	token_print_line(current_token);
	
	return current_token;
}

#define peek_type(parser)          peek_type_impl((parser), true,  __FUNCTION__, __LINE__)
#define peek_type_with_eos(parser) peek_type_impl((parser), false, __FUNCTION__, __LINE__)
token_type_t peek_type_impl(parser_p parser, bool ignore_ws_eos, const char* caller, int line) {
	return peek_impl(parser, ignore_ws_eos, caller, line)->type;
}

#define consume_type(parser, type) consume_type_impl((parser), (type), __FUNCTION__, __LINE__)
token_p consume_type_impl(parser_p parser, token_type_t type, const char* caller, int line) {
	token_p t = consume_impl(parser, (type != T_WS_EOS), caller, line);
	if (t->type == type)
		return t;
	
	token_p current_token = &parser->file->tokens[parser->pos];
	
	char token_desc[128];
	token_dump(current_token, token_desc, sizeof(token_desc));
	printf("%s:%d consume_type, expected %d, got %s\n", caller, line, type, token_desc);
	token_print_line(current_token);
	abort();
}



//
// Definitions
//

/*
tree_p parse_module(tokenized_file_p file) {
	parser_t parser_storage = { file, 0 };
	parser_p parser = &parser_storage;
	
	token_p t;
	do {
		t = consume(parser);
	} while (t->type != T_EOF);
	
	return NULL;
}
*/

tree_p parse_func_def(parser_p parser);
tree_p parse_var_def(parser_p parser);
bool is_stmt_start(token_type_t type);
tree_p parse_stmt(parser_p parser);
bool is_expr_start(token_type_t type);
tree_p parse_expr(parser_p parser);


tree_p parse_module(tokenized_file_p file) {
	parser_t parser_storage = { file, 0 };
	parser_p parser = &parser_storage;
	
	printf("module:\n");
	while (true) {
		token_type_t type = peek_type(parser);
		if ( type == T_FUNC ) {
			parse_func_def(parser);
		} else if ( is_expr_start(type) ) {
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
	token_p id = consume_type(parser, T_ID);
	printf(" id %.*s:\n", id->size, id->source);
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
	consume_type(parser, T_WS_EOS);
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
	token_p t = consume(parser);
	switch(t->type) {
		case T_ID:
			printf("      expr id: %.*s\n", t->size, t->source);
			break;
		case T_INT:
			printf("      expr int: %d\n", t->int_val);
			break;
		case T_STR:
			printf("      expr str: \"%.*s\"\n", t->str_size, t->str_val);
			break;
		case T_RBO:
			printf("      expr paren:\n");
			parse_expr(parser);
			consume_type(parser, T_RBC);
			break;
		default:
			abort();
			return NULL;
	}
	
	if (peek_type(parser) == T_ID) {
		// Got an operator
		consume_type(parser, T_ID);
		parse_expr(parser);
		return NULL;
	}
	
	return NULL;
}
