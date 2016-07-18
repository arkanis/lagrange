#include "common.h"


//
// Parser state and utility stuff
//

struct parser_s {
	module_p module;
	size_t   pos;
	
	list_t(token_type_t) tried_token_types;
	FILE* error_stream;
};


static token_p next_filtered_token(parser_p parser, size_t start_pos, bool ignore_line_breaks) {
	size_t pos = start_pos;
	while (pos < parser->module->tokens.len) {
		token_p token = &parser->module->tokens.ptr[pos];
		pos++;
		
		// Skip whitespace and comment tokens
		if ( token->type == T_WS || token->type == T_COMMENT )
			continue;
		// Also skip whitespaces with newlines if we're told to do so
		if ( token->type == T_WSNL && ignore_line_breaks )
			continue;
		
		// Return the first token we didn't skip
		return token;
	}
	
	// We're either beyond the last token or found no filtered token beyond pos
	return NULL;
}

static void parser_error(parser_p parser, const char* message) {
	token_p token = next_filtered_token(parser, parser->pos, true);
	
	fprintf(parser->error_stream, "%s:%d:%d: ", parser->module->filename,
		token_line(parser->module, token),
		token_col(parser->module, token)
	);
	
	if (message) {
		fputs(message, parser->error_stream);
		fputs("\n", parser->error_stream);
	}
	
	fputs("expected", parser->error_stream);
	for(size_t i = 0; i < parser->tried_token_types.len; i++) {
		char* desc = token_desc(parser->tried_token_types.ptr[i]);
		if (!desc)
			desc = token_type_name(parser->tried_token_types.ptr[i]);
		
		fprintf(parser->error_stream, " %s", desc);
		if (i != parser->tried_token_types.len - 1)
			fputs(",", parser->error_stream);
	}
	
	fputs(" before ", parser->error_stream);
	token_print(parser->error_stream, token, TP_INLINE_DUMP);
	fputs("\n", parser->error_stream);
	
	size_t token_index = token - parser->module->tokens.ptr;
	token_print_range(parser->error_stream, parser->module, token_index, 1);
}

static token_p try(parser_p parser, token_type_t type) {
	bool already_tried = false;
	for(size_t i = 0; i < parser->tried_token_types.len; i++) {
		if (parser->tried_token_types.ptr[i] == type) {
			already_tried = true;
			break;
		}
	}
	
	if (!already_tried)
		list_append(&parser->tried_token_types, type);
	
	token_p token = next_filtered_token(parser, parser->pos, (type == T_WSNL) ? false : true);
	if (token && token->type == type)
		return token;
	return NULL;
}

static token_p try_after_token(parser_p parser, token_type_t type, token_p start_token) {
	if (start_token == NULL)
		return try(parser, type);
	
	// Don't log look ahead tokens for error messages right now. Not sure if
	// this information is helpful to the user.
	size_t start_token_pos = start_token - parser->module->tokens.ptr + 1;
	if ( start_token_pos >= parser->module->tokens.len ) {
		fprintf(stderr, "try_after_token(): Start token not part of the currently parsed module!\n");
		abort();
	}
	
	token_p token = next_filtered_token(parser, start_token_pos, (type == T_WSNL) ? false : true);
	if (token && token->type == type)
		return token;
	return NULL;
}


static token_p consume(parser_p parser, token_p token) {
	if (token == NULL) {
		fprintf(stderr, "consume(): Tried to consume NULL!\n");
		abort();
	}
	
	// Since index is unsigned negative indices will wrap around to very large
	// indices that are out of bounds, too. So the next if catches them as well.
	size_t index = token - parser->module->tokens.ptr;
	if ( index >= parser->module->tokens.len ) {
		fprintf(stderr, "consume(): Token not part of the currently parsed module!\n");
		abort();
	}
	
	// Advance parser position and clear tried token types
	parser->pos = index + 1;
	list_destroy(&parser->tried_token_types);
	
	return token;
}

static token_p try_consume(parser_p parser, token_type_t type) {
	token_p token = try(parser, type);
	if (token)
		consume(parser, token);
	return token;
}

static token_p consume_type(parser_p parser, token_type_t type) {
	token_p token = try(parser, type);
	if (!token) {
		parser_error(parser, NULL);
		abort();
	}
	return consume(parser, token);
}



//
// Public parser interface to parse a rule
//

node_p parse(module_p module, parser_rule_func_t rule, FILE* error_stream) {
	parser_t parser = (parser_t){
		.module       = module,
		.error_stream = error_stream,
		.pos          = 0
	};
	
	node_p node = rule(&parser);
	consume_type(&parser, T_EOF);
	
	list_destroy(&parser.tried_token_types);
	parser.error_stream = NULL;
	return node;
}



//
// Try functions for different rules
//

void parse_stmts(parser_p parser, node_p parent, node_list_p list);

static token_p try_stmt(parser_p parser);
static node_p  complete_parser_var_def_statement(parser_p parser, node_p cexpr);
static node_p  complete_parser_expr_statement(parser_p parser, node_p cexpr);
static token_p try_eos(parser_p parser, token_p after_token);
static token_p consume_eos(parser_p parser);

static token_p try_cexpr(parser_p parser);
       node_p  parse_cexpr(parser_p parser);
static node_p  complete_parser_expr(parser_p parser, node_p cexpr);



//
// Parser rules
//

node_p parse_module(parser_p parser) {
	node_p node = node_alloc(NT_MODULE);
	node->tokens = parser->module->tokens;
	
	while ( ! try(parser, T_EOF) ) {
		node_p def = NULL;
		
		if ( try(parser, T_FUNC) ) {
			def = parse_func_def(parser);
		} else {
			parser_error(parser, NULL);
			abort();
		}
		
		node_append(node, &node->module.defs, def);
	}
	
	return node;
}


//
// Definitions
//

node_p parse_func_def(parser_p parser) {
	// def     = "func" ID [ def-mod ] "{" [ stmt ] "}"
	// def-mod = ( "in" | "out" )  "(" ID ID? [ "," ID ID? ] ")"
	node_p node = node_alloc(NT_FUNC);
	
	token_p t = consume_type(parser, T_FUNC);
	node_first_token(node, t);
	node->func.name = consume_type(parser, T_ID)->source;
	
	while ( (t = try_consume(parser, T_IN)) || (t = try_consume(parser, T_OUT)) ) {
		node_list_p arg_list = NULL;
		if ( t->type == T_IN )
			arg_list = &node->func.in;
		else if ( t->type == T_OUT )
			arg_list = &node->func.out;
		else
			abort();
		
		consume_type(parser, T_RBO);
		while ( !try(parser, T_RBC) ) {
			node_p arg = node_alloc_append(NT_ARG, node, arg_list);
			
			node_p type_expr = parse_cexpr(parser);
			node_set(arg, &arg->arg.type_expr, type_expr);
			
			// Set the arg name if we got an ID after the type. Otherwise leave
			// the arg unnamed (nulled out)
			if ( try(parser, T_ID) )
				arg->arg.name = consume_type(parser, T_ID)->source;
			
			if ( !try_consume(parser, T_COMMA) )
				break;
		}
		consume_type(parser, T_RBC);
	}
	
	if ( try_consume(parser, T_CBO) ) {
		parse_stmts(parser, node, &node->func.body);
		t = consume_type(parser, T_CBC);
	} else if ( try_consume(parser, T_DO) ) {
		parse_stmts(parser, node, &node->func.body);
		t = consume_type(parser, T_END);
	} else {
		parser_error(parser, NULL);
		abort();
	}
	
	node_last_token(node, t);
	return node;
}

void parse_stmts(parser_p parser, node_p parent, node_list_p list) {
	while ( try_stmt(parser) ) {
		node_p stmt = parse_stmt(parser);
		node_append(parent, list, stmt);
	}
}



//
// Statements
//

static token_p try_stmt(parser_p parser) {
	token_p t = NULL;
	if      ( (t = try(parser, T_CBO))    ) return t;
	else if ( (t = try(parser, T_DO))     ) return t;
	else if ( (t = try(parser, T_WHILE))  ) return t;
	else if ( (t = try(parser, T_IF))     ) return t;
	else if ( (t = try(parser, T_RETURN)) ) return t;
	else if ( (t = try_cexpr(parser))     ) return t;
	
	return NULL;
}

node_p parse_stmt(parser_p parser) {
	token_p t = NULL;
	node_p node = NULL;
	
	if ( (t = try_consume(parser, T_CBO)) || (t = try_consume(parser, T_DO)) ) {
		// stmt = "{"  [ stmt ] "}"
		//        "do" [ stmt ] "end"
		node = node_alloc(NT_SCOPE);
		node_first_token(node, t);
		
		while ( try_stmt(parser) )
			node_append(node, &node->scope.stmts, parse_stmt(parser) );
		
		t = consume_type(parser, (t->type == T_CBO) ? T_CBC : T_END);
		node_last_token(node, t);
	} else if ( (t = try_consume(parser, T_WHILE)) ) {
		// stmt = "while" expr "do" [ stmt ] "end"
		//                     "{"  [ stmt ] "}"
		//                     WSNL [ stmt ] "end"  // check as last alternative, see note 1
		node = node_alloc(NT_WHILE_STMT);
		node_first_token(node, t);
		
		node_p cond = parse_expr(parser);
		node_set(node, &node->while_stmt.cond, cond);
		
		t = try_consume(parser, T_DO);
		if (!t) t = try_consume(parser, T_CBO);
		if (!t) t = try_consume(parser, T_WSNL);
		if (!t) {
			parser_error(parser, "while needs a block as body!");
			abort();
		}
		
		while ( try_stmt(parser) )
			node_append(node, &node->while_stmt.body, parse_stmt(parser) );
		
		t = consume_type(parser, (t->type == T_DO || t->type == T_WSNL) ? T_END : T_CBC );
		node_last_token(node, t);
	} else if ( (t = try_consume(parser, T_IF)) ) {
		// "if" expr "do" [ stmt ]     ( "else"     [ stmt ] )? "end"
		//           "{"  [ stmt ] "}" ( "else" "{" [ stmt ] "}" )?
		//           WSNL [ stmt ]     ( "else"     [ stmt ] )? "end"  // check as last alternative, see note 1
		node = node_alloc(NT_IF_STMT);
		node_first_token(node, t);
		
		node_p cond = parse_expr(parser);
		node_set(node, &node->if_stmt.cond, cond);
		
		token_p block_token = try_consume(parser, T_DO);
		if (!block_token) block_token = try_consume(parser, T_CBO);
		if (!block_token) block_token = try_consume(parser, T_WSNL);
		if (!block_token) {
			parser_error(parser, "if needs a block as body!");
			abort();
		}
		
		while ( try_stmt(parser) )
			node_append(node, &node->if_stmt.true_case, parse_stmt(parser) );
		
		if (block_token->type == T_CBO)
			t = consume_type(parser, T_CBC);
		
		if ( try_consume(parser, T_ELSE) ) {
			if (block_token->type == T_CBO)
				consume_type(parser, T_CBO);
			
			while ( try_stmt(parser) )
				node_append(node, &node->if_stmt.false_case, parse_stmt(parser) );
			
			if (block_token->type == T_CBO)
				t = consume_type(parser, T_CBC);
		}
		
		if (block_token->type == T_DO || block_token->type == T_WSNL)
			t = consume_type(parser, T_END);
		
		node_last_token(node, t);
	} else if ( (t = try_consume(parser, T_RETURN)) ) {
		// "return" ( expr ["," expr] )? eos
		node = node_alloc(NT_RETURN_STMT);
		node_first_token(node, t);
		
		if ( !try_eos(parser, NULL) ) {
			node_p expr = parse_expr(parser);
			node_append(node, &node->return_stmt.args, expr);
			
			while ( try_consume(parser, T_COMMA) ) {
				expr = parse_expr(parser);
				node_append(node, &node->return_stmt.args, expr);
			}
		}
		
		t = consume_eos(parser);
		node_last_token(node, t);
	} else if ( (t = try_cexpr(parser)) ) {
		// cexpr ID ( "=" expr )? [ "," ID ( "=" expr )? ]  // how to differ between ID and binary_op, see note 2
		// cexpr [ binary_op cexpr ] "while" expr eos
		//                           "if"    expr eos
		//                           eos
		node_p cexpr = parse_cexpr(parser);
		
		if ( (t = try(parser, T_ID)) ) {
			// In case of an ID we use a lookahead to decide if it's user
			// defined operator or the name of a variable that gets defined.
			if (
				try_after_token(parser, T_ASSIGN, t) ||
				try_after_token(parser, T_COMMA, t)  ||
				try_eos(parser, t)
			) {
				node = complete_parser_var_def_statement(parser, cexpr);
			} else {
				node = complete_parser_expr_statement(parser, cexpr);
			}
		} else {
			// has to be the expr statement case (with optional binary_op, etc.)
			node = complete_parser_expr_statement(parser, cexpr);
		}
	} else {
		parser_error(parser, NULL);
		abort();
	}
	
	return node;
}

static node_p complete_parser_var_def_statement(parser_p parser, node_p cexpr) {
	// The first cexpr is already consumed and passed to us as parameter
	// cexpr ID ( "=" expr )? [ "," ID ( "=" expr )? ]  // how to differ between ID and binary_op, see note 2
	node_p node = node_alloc(NT_VAR);
	node_first_token(node, cexpr->tokens.ptr);
	
	node_set(node, &node->var.type_expr, cexpr);
	node_p binding = NULL;
	token_p t = NULL;
	
	do {
		binding = node_alloc_append(NT_BINDING, node, &node->var.bindings);
		t = consume_type(parser, T_ID);
		binding->binding.name = t->source;
		node_first_token(binding, t);
		
		if ( (t = try_consume(parser, T_ASSIGN)) ) {
			node_p expr = parse_expr(parser);
			node_set(binding, &binding->binding.value, expr);
			
			node_last_token(node, expr->tokens.ptr + expr->tokens.len - 1);
		}
	} while ( try_consume(parser, T_COMMA) );
	
	t = consume_eos(parser);
	node_last_token(node, t);
	
	return node;
}

static node_p complete_parser_expr_statement(parser_p parser, node_p cexpr) {
	// The first cexpr is already consumed and passed to us as parameter
	// cexpr [ binary_op cexpr ] "while" expr eos
	//                           "if"    expr eos
	//                           eos
	node_p node = complete_parser_expr(parser, cexpr);
	
	if ( try_eos(parser, NULL) ) {
		// Just skip all the other cases
	} else if ( try_consume(parser, T_WHILE) ) {
		node_p body = node;
		node = node_alloc(NT_WHILE_STMT);
		node_first_token(node, body->tokens.ptr);
		
		node_p cond = parse_expr(parser);
		node_set(node, &node->while_stmt.cond, cond);
		
		node_append(node, &node->while_stmt.body, body);
	} else if ( try_consume(parser, T_IF) ) {
		node_p body = node;
		node = node_alloc(NT_IF_STMT);
		node_first_token(node, body->tokens.ptr);
		
		node_p cond = parse_expr(parser);
		node_set(node, &node->if_stmt.cond, cond);
		
		node_append(node, &node->if_stmt.true_case, body);
	}
	
	token_p t = consume_eos(parser);
	node_last_token(node, t);
	
	return node;
}


static token_p try_eos(parser_p parser, token_p after_token) {
	token_p t = NULL;
	if      ( (t = try_after_token(parser, T_EOF,  after_token)) ) return t;
	else if ( (t = try_after_token(parser, T_SEMI, after_token)) ) return t;
	else if ( (t = try_after_token(parser, T_CBC,  after_token)) ) return t;
	else if ( (t = try_after_token(parser, T_END,  after_token)) ) return t;
	else if ( (t = try_after_token(parser, T_ELSE, after_token)) ) return t;
	else if ( (t = try_after_token(parser, T_WSNL, after_token)) ) return t;
	return NULL;
}

static token_p consume_eos(parser_p parser) {
	token_p t = NULL;
	if      ( (t = try(parser, T_EOF))          ) return t;
	else if ( (t = try_consume(parser, T_SEMI)) ) return t;
	else if ( (t = try(parser, T_CBC))          ) return t;
	else if ( (t = try(parser, T_END))          ) return t;
	else if ( (t = try(parser, T_ELSE))         ) return t;
	else if ( (t = try_consume(parser, T_WSNL)) ) return t;
	return NULL;
}



//
// Expressions
//

static token_p try_cexpr(parser_p parser) {
	token_p t = NULL;
	if      ( (t = try(parser, T_ID)) ) return t;
	else if ( (t = try(parser, T_INT)) ) return t;
	else if ( (t = try(parser, T_STR)) ) return t;
	else if ( (t = try(parser, T_RBO)) ) return t;
	#define UNARY_OP(token, id, name)  \
		else if ( (t = try(parser, token)) ) return t;
	#include "op_spec.h"
	
	return NULL;
}

node_p parse_cexpr(parser_p parser) {
	token_p t = NULL;
	node_p node = NULL;
	
	if ( (t = try_consume(parser, T_ID)) ) {
		node = node_alloc(NT_ID);
		node_first_token(node, t);
		node->id.name = t->source;
	} else if ( (t = try_consume(parser, T_INT)) ) {
		node = node_alloc(NT_INTL);
		node_first_token(node, t);
		node->intl.value = t->int_val;
	} else if ( (t = try_consume(parser, T_STR)) ) {
		node = node_alloc(NT_STRL);
		node_first_token(node, t);
		node->strl.value = t->str_val;
	} else if ( (t = try_consume(parser, T_RBO)) ) {
		node = parse_expr(parser);
		node_first_token(node, t);
		t = consume_type(parser, T_RBC);
		node_last_token(node, t);
	
	// cexpr = unary_op cexpr
	#define UNARY_OP(token, id, name)                     \
		} else if ( (t = try_consume(parser, token)) ) {  \
			node = node_alloc(NT_UNARY_OP);               \
			node_first_token(node, t);                    \
			node->unary_op.index = id;                    \
			node_p arg = parse_cexpr(parser);             \
			node_set(node, &node->unary_op.arg, arg);
	#include "op_spec.h"
	
	} else {
		parser_error(parser, NULL);
		abort();
	}
	
	// One complete cexpr parsed, now process the trailing stuff.
	// Since we can chain together any number of cexpr with that trailing stuff
	// (we're left recursive) we have to do this in a loop here.
	while (true) {
		if ( (t = try_consume(parser, T_RBO)) ) {
			// cexpr = cexpr "(" ( expr [ "," expr ] )? ")"
			node_p target_expr = node;
			node = node_alloc(NT_CALL);
			node_first_token(node, target_expr->tokens.ptr);
			node_set(node, &node->call.target_expr, target_expr);
			
			if ( !try(parser, T_RBC) ) {
				node_p expr = parse_expr(parser);
				node_append(node, &node->call.args, expr);
				
				while ( try_consume(parser, T_COMMA) ) {
					expr = parse_expr(parser);
					node_append(node, &node->call.args, expr);
				}
			}
			
			t = consume_type(parser, T_RBC);
			node_last_token(node, t);
		} else if ( (t = try_consume(parser, T_SBO)) ) {
			// cexpr "[" ( expr [ "," expr ] )? "]"
			node_p target_expr = node;
			node = node_alloc(NT_INDEX);
			node_first_token(node, target_expr->tokens.ptr);
			node_set(node, &node->index.target_expr, target_expr);
			
			if ( !try(parser, T_SBC) ) {
				node_p expr = parse_expr(parser);
				node_append(node, &node->index.args, expr);
				
				while ( try_consume(parser, T_COMMA) ) {
					expr = parse_expr(parser);
					node_append(node, &node->index.args, expr);
				}
			}
			
			t = consume_type(parser, T_SBC);
			node_last_token(node, t);
		} else if ( (t = try_consume(parser, T_PERIOD)) ) {
			// cexpr "." ID
			node_p aggregate = node;
			node = node_alloc(NT_MEMBER);
			node_first_token(node, aggregate->tokens.ptr);
			node_set(node, &node->member.aggregate, aggregate);
			
			t = consume_type(parser, T_ID);
			node->member.member = t->source;
			
			node_last_token(node, t);
		} else {
			break;
		}
	}
	
	return node;
}

static token_p try_binary_op(parser_p parser) {
	token_p t = NULL;
	if ( (t = try(parser, T_ID)) ) return t;
	#define BINARY_OP(token, id, name)  \
		else if ( (t = try(parser, token)) ) return t;
	#include "op_spec.h"
	
	return NULL;
}

static node_p complete_parser_expr(parser_p parser, node_p cexpr) {
	// The first cexpr is already consumed and passed to us as parameter
	// cexpr [ binary_op cexpr ]
	node_p node = cexpr;
	
	token_p t = NULL;
	if ( try_binary_op(parser) && !try_eos(parser, NULL) ) {
		// Got an operator, wrap everything into an uops node and collect the
		// remaining operators and expressions.
		node_p cexpr = node;
		node = node_alloc(NT_UOPS);
		
		node_first_token(node, cexpr->tokens.ptr);
		node_append(node, &node->uops.list, cexpr);
		node_last_token(node, cexpr->tokens.ptr + cexpr->tokens.len - 1);
		
		while ( (t = try_binary_op(parser)) && !try_eos(parser, NULL) ) {
			consume(parser, t);
			
			// TODO: Store the token in the node, the resolve uops pass can then
			// look at the token to figure out which operator it was.
			node_p op_node = node_alloc_append(NT_ID, node, &node->uops.list);
			node_first_token(op_node, t);
			op_node->id.name = t->source;
			
			cexpr = parse_cexpr(parser);
			node_append(node, &node->uops.list, cexpr);
			node_last_token(node, cexpr->tokens.ptr + cexpr->tokens.len - 1);
		}
	}
	
	return node;
}

node_p parse_expr(parser_p parser) {
	// cexpr [ binary_op cexpr ]
	node_p cexpr = parse_cexpr(parser);
	return complete_parser_expr(parser, cexpr);
}