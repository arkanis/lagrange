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


static token_p next_filtered_token(parser_p parser, bool ignore_line_breaks) {
	size_t pos = parser->pos;
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
	token_p token = &parser->module->tokens.ptr[parser->pos];
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
	
	fputs(" after ", parser->error_stream);
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
	
	token_p token = next_filtered_token(parser, (type == T_WSNL) ? false : true);
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
	if (!token)
		parser_error(parser, NULL);
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
	
	list_destroy(&parser.tried_token_types);
	parser.error_stream = NULL;
	return node;
}

/*

//
// Parser rules
//

void parse_stmts(parser_p parser, node_p parent, node_list_p list);


node_p parse_module(parser_p parser) {
	node_p node = node_alloc(NT_MODULE);
	node->tokens = parser->module->tokens;
	
	while ( ! try_consume(parser, T_EOF) ) {
		node_p def = NULL;
		
		if ( try(parser, T_FUNC) ) {
			def = parse_func_def(parser);
		} else {
			parser_error(parser, NULL);
			return node;
		}
		
		node_append(node, &node->module.defs, def);
	}
	
	return node;
}


//
// Definitions
//

node_p parse_func_def(parser_p parser) {
	node_p node = node_alloc(NT_FUNC);
	
	consume_type(parser, T_FUNC);
	node->func.name = consume_type(parser, T_ID)->source;
	
	token_p t = NULL;
	if ( (t = try_consume(parser, T_IN)) || (t = try_consume(parser, T_OUT)) ) {
		node_list_p arg_list = NULL;
		if ( t->type == T_IN )
			arg_list = &node->func.in;
		else if ( t->type == T_OUT )
			arg_list = &node->func.out;
		else
			abort();
		
		consume_type(parser, T_RBO);
		while ( !try(parser, T_RBC) ) {
			node_p arg = node_alloc(NT_ARG);
			
			t = consume_type(parser, T_ID);
			node_p type_expr = node_alloc_set(NT_ID, arg, &arg->arg.type_expr);
			type_expr->id.name = t->src;
			
			// Set the arg name if we got an ID after the type. Otherwise leave
			// the arg unnamed (nulled out)
			if ( try(parser, T_ID) )
				arg->arg.name = consume_type(parser, T_ID)->src;
			
			node_append(node, arg_list, arg);
			
			if ( !try_consume(parser, T_COMMA) )
				break;
		}
		consume_type(parser, T_RBC);
		
	} else if ( try_consume(parser, T_CBO) ) {
		parse_stmts(parser, node, &node->func.body);
		consume_type(parser, T_CBC);
	} else if ( try_consume(parser, T_DO) ) {
		parse_stmts(parser, node, &node->func.body);
		consume_type(parser, T_END);
	}
	
	return node;
}

void parse_stmts(parser_p parser, node_p parent, node_list_p list) {
	while ( try_stmt_start(parser) ) {
		node_p stmt = parse_stmt(parser);
		node_append(parent, list, stmt);
	}
}

*/

//
// Expressions
//

node_p parse_expr(parser_p parser) {
	token_p t = NULL;
	node_p node = NULL;
	
	if ( (t = try_consume(parser, T_ID)) ) {
		node = node_alloc(NT_ID);
		node->id.name = t->source;
	} else if ( (t = try_consume(parser, T_INT)) ) {
		node = node_alloc(NT_INTL);
		node->intl.value = t->int_val;
	} else if ( (t = try_consume(parser, T_STR)) ) {
		node = node_alloc(NT_STRL);
		node->strl.value = t->str_val;
	} else {
		parser_error(parser, NULL);
		abort();
	}
	
	return node;
}