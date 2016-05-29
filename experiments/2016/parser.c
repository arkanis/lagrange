#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "parser.h"


//
// Parser state and utility stuff
//

struct parser_s {
	token_list_p list;
	size_t pos;
};


int next_filtered_token_at(parser_p parser, size_t pos, bool ignore_ws_eos) {
	size_t offset = 0;
	while (pos + offset < (size_t)parser->list->tokens.len) {
		token_type_t type = parser->list->tokens.ptr[pos + offset].type;
		// Return the offset if we found a filtered token there
		if ( !( type == T_WS || type == T_COMMENT || (ignore_ws_eos && type == T_WSNL) ) )
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
	/* For now return T_EOF (the last token in the list) so we can consume as
	   many EOFs as we want.
	// Return NULL if there are no more filtered tokens
	if (offset == -1)
		return NULL;
	*/
	
	token_p current_token;
	if (offset == -1) {
		current_token = &parser->list->tokens.ptr[parser->list->tokens.len - 1];
	} else {
		parser->pos += offset;
		current_token = &parser->list->tokens.ptr[parser->pos];
		parser->pos++;
	}
	
	printf("%s:%d consume ", caller, line);
	token_print(stdout, current_token, TP_INLINE_DUMP);
	printf("\n");
	
	return current_token;
}

#define peek(parser)          peek_impl((parser), true,  __FUNCTION__, __LINE__)
#define peek_with_eos(parser) peek_impl((parser), false, __FUNCTION__, __LINE__)
token_p peek_impl(parser_p parser, bool ignore_ws_eos, const char* caller, int line) {
	int offset = next_filtered_token_at(parser, parser->pos, ignore_ws_eos);
	/* For now return T_EOF (the last token in the list) so we can consume as
	   many EOFs as we want.
	// Return NULL if there are no more filtered tokens
	if (offset == -1)
		return NULL;
	*/
	
	token_p current_token;
	if (offset == -1)
		current_token = &parser->list->tokens.ptr[parser->list->tokens.len - 1];
	else
		current_token = &parser->list->tokens.ptr[parser->pos + offset];
	
	printf("%s:%d peek ", caller, line);
	token_print(stdout, current_token, TP_INLINE_DUMP);
	printf("\n");
	
	return current_token;
}

#define peek_type(parser)          peek_type_impl((parser), true,  __FUNCTION__, __LINE__)
#define peek_type_with_eos(parser) peek_type_impl((parser), false, __FUNCTION__, __LINE__)
token_type_t peek_type_impl(parser_p parser, bool ignore_ws_eos, const char* caller, int line) {
	return peek_impl(parser, ignore_ws_eos, caller, line)->type;
}

#define consume_type(parser, type) consume_type_impl((parser), (type), __FUNCTION__, __LINE__)
token_p consume_type_impl(parser_p parser, token_type_t type, const char* caller, int line) {
	token_p t = consume_impl(parser, (type != T_WSNL), caller, line);
	if (t->type == type)
		return t;
	
	token_p current_token = &parser->list->tokens.ptr[parser->pos-1];
	
	printf("%s:%d consume_type, expected %d, got ", caller, line, type);
	token_print(stdout, current_token, TP_INLINE_DUMP);
	printf("\n");
	
	abort();
}



//
// Parser rules
//

static bool is_stmt_start(token_type_t type);
static bool is_expr_start(token_type_t type);

node_p parse(token_list_p list, parser_rule_func_t rule) {
	parser_t parser = { list, 0 };
	return rule(&parser);
}


//
// Definitions
//

node_p parse_module(parser_p parser) {
	node_p node = node_alloc(NT_MODULE);
	while ( is_stmt_start(peek_type(parser)) ) {
		node_p stmt = parse_stmt(parser);
		node_append(node, &node->module.stmts, stmt);
	}
	
	consume_type(parser, T_EOF);
	return node;
}


//
// Statements
//

static bool is_stmt_start(token_type_t type) {
	switch(type) {
		case T_SYSCALL:
		case T_VAR:
		case T_CBO:
		case T_IF:
			return true;
		default:
			return is_expr_start(type);
	}
}

bool peek_eos(parser_p parser) {
	token_type_t type = peek_type_with_eos(parser);
	return (type == T_WSNL || type == T_EOF);
}

void parse_eos(parser_p parser) {
	token_p t = consume_with_eos(parser);
	if ( !(t->type == T_WSNL || t->type == T_EOF) ) {
		printf("%s:%d:%d: expectet WSNL or EOF, got:\n", parser->list->filename, token_line(t), token_col(t));
		token_print_line(stderr, t, 0);
		abort();
	}
}

node_p parse_stmt(parser_p parser) {
	if ( is_expr_start(peek_type(parser)) )
		return parse_expr(parser);
	
	node_p stmt = NULL;
	token_p t = consume(parser);
	switch (t->type) {
		case T_SYSCALL:
			// syscall eos
			// syscall expr ["," expr] eos
			stmt = node_alloc(NT_SYSCALL);
			
			if ( is_expr_start(peek_type_with_eos(parser)) ) {
				node_p expr = parse_expr(parser);
				node_append(stmt, &stmt->syscall.args, expr);
				
				while ( peek_type_with_eos(parser) == T_COMMA ) {
					consume_type(parser, T_COMMA);
					expr = parse_expr(parser);
					node_append(stmt, &stmt->syscall.args, expr);
				}
			}
			
			parse_eos(parser);
			return stmt;
		case T_VAR:
			// var ID eos
			// var ID = expr eos
			stmt = node_alloc(NT_VAR);
			token_p id = consume_type(parser, T_ID);
			stmt->var.name = id->src;
			
			if ( peek_type_with_eos(parser) == T_ASSIGN ) {
				consume_type(parser, T_ASSIGN);
				node_p expr = parse_expr(parser);
				node_set(stmt, &stmt->var.value, expr);
			}
			
			return stmt;
		case T_CBO:
			// "{" [ stmts ] "}"
			stmt = node_alloc(NT_SCOPE);
			
			while ( is_stmt_start(peek_type(parser)) )
				node_append(stmt, &stmt->scope.stmts, parse_stmt(parser) );
			consume_type(parser, T_CBC);
			
			return stmt;
		case T_IF:
			// "if" expr ( eos | "then" ) stmt ( "else" stmt )?
			stmt = node_alloc(NT_IF);
			
			{
				node_p cond = parse_expr(parser);
				node_set(stmt, &stmt->if_stmt.cond, cond);
				
				if ( peek_type_with_eos(parser) == T_THEN )
					consume_type(parser, T_THEN);
				else
					parse_eos(parser);
				
				node_p true_case = parse_stmt(parser);
				node_set(stmt, &stmt->if_stmt.true_case, true_case);
				
				if ( peek_type(parser) == T_ELSE ) {
					consume_type(parser, T_ELSE);
					node_p false_case = parse_stmt(parser);
					node_set(stmt, &stmt->if_stmt.false_case, false_case);
				}
			}
			
			return stmt;
		
		default:
			printf("%s:%d:%d: expectet SYSCALL, VAR or expr, got:\n", parser->list->filename, token_line(t), token_col(t));
			token_print_line(stderr, t, 0);
			abort();
	}
	
	return NULL;
}


//
// Expressions
//

static bool is_expr_start(token_type_t type) {
	switch(type) {
		case T_ID:
		case T_INT:
		case T_STR:
		case T_RBO:
			return true;
		// unary operators
		case T_ADD:
		case T_SUB:
		case T_NOT:
		case T_COMPL:
			return true;
		default:
			return false;
	}
}

node_p parse_expr_without_trailing_ops(parser_p parser) {
	node_p node = NULL;
	
	token_p t = consume(parser);
	switch(t->type) {
		case T_ID:
			node = node_alloc(NT_ID);
			node->id.name = t->src;
			break;
		case T_INT:
			node = node_alloc(NT_INTL);
			node->intl.value = t->int_val;
			break;
		case T_STR:
			node = node_alloc(NT_STRL);
			node->strl.value = t->str_val;
			break;
		case T_RBO:
			// TODO: remember somehow that this node was surrounded by brackets!
			// Otherwise exact syntax reconstruction will be impossible.
			node = parse_expr(parser);
			consume_type(parser, T_RBC);
			break;
		
		case T_ADD:
		case T_SUB:
		case T_NOT:
		case T_COMPL:
			node = node_alloc(NT_UNARY_OP);
			node->unary_op.type = t->type;
			node_set(node, &node->unary_op.arg, parse_expr_without_trailing_ops(parser) );
			break;
		
		default:
			printf("%s:%d:%d: expectet ID, INT or STR, got:\n", parser->list->filename, token_line(t), token_col(t));
			token_print_line(stderr, t, 0);
			abort();
			return NULL;
	}
	
	return node;
}

static bool is_binary_op(token_type_t type) {
	switch(type) {
		// use defined operator
		case T_ID:
			return true;
		
		// builtin binary operators
		case T_ADD: case T_ADD_ASSIGN:  // + +=
		case T_SUB: case T_SUB_ASSIGN:  // - -=
		case T_MUL: case T_MUL_ASSIGN:  // * *=
		case T_DIV: case T_DIV_ASSIGN:  // / /=
		case T_MOD: case T_MOD_ASSIGN:  // % %=
		
		case T_LT: case T_LE: case T_SL: case T_SL_ASSIGN:  // < <= << <<=
		case T_GT: case T_GE: case T_SR: case T_SR_ASSIGN:  // > >= >> >>=
		
		case T_BIN_AND: case T_BIN_AND_ASSIGN:  // & &=
		case T_BIN_XOR: case T_BIN_XOR_ASSIGN:  // ^ ^=
		case T_BIN_OR:  case T_BIN_OR_ASSIGN:   // | |=
		
		case T_ASSIGN: case T_EQ:   // = ==
		case T_NEQ: case T_PERIOD:  // != .
		
		case T_NOT: case T_AND: case T_OR:
			return true;
		
		default:
			return false;
	}
}


node_p parse_expr(parser_p parser) {
	node_p node = parse_expr_without_trailing_ops(parser);
	
	token_type_t type = peek_type_with_eos(parser);
	if ( is_binary_op(type) ) {
		// Got an operator, wrap everything into an uops node and collect the
		// remaining operators and expressions.
		node_p uops = node_alloc(NT_UOPS);
		node_append(uops, &uops->uops.list, node);
		
		while ( is_binary_op(type = peek_type_with_eos(parser)) ) {
			token_p id = consume_type(parser, type);
			
			node_p op_node = node_alloc_append(NT_ID, uops, &uops->uops.list);
			op_node->id.name = id->src;
			
			node_p expr = parse_expr_without_trailing_ops(parser);
			node_append(uops, &uops->uops.list, expr);
		}
		
		return uops;
	}
	
	return node;
}