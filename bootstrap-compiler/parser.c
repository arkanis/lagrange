#include "common.h"


//
// Parser state and utility stuff
//

struct parser_s {
	module_p module;
	size_t pos;
	
	list_t(token_type_t) tried_token_types;
	FILE* error_stream;
};


static void parser_error(parser_p parser, const char* message) {
	token_p t = &parser->module->tokens.ptr[parser->pos];
	fprintf(parser->error_stream, "%s:%d:%d: ", parser->module->filename,
		token_line(parser->module, t),
		token_col(parser->module, t)
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
	
	fputs(" got ", parser->error_stream);
	token_print(parser->error_stream, t, TP_INLINE_DUMP);
	fputs("\n", parser->error_stream);
	
	token_print_range(parser->error_stream, parser->module, parser->pos, 1);
}

static int next_filtered_token_at(parser_p parser, size_t pos, bool end_of_statement_possible) {
	size_t offset = 0;
	while (pos + offset < parser->module->tokens.len) {
		token_type_t type = parser->module->tokens.ptr[pos + offset].type;
		offset++;
		
		// Skip whitespace and comment tokens
		if ( type == T_WS || type == T_COMMENT )
			continue;
		// Also skip whitespaces with newlines if we're not at a possible end
		// of statement.
		if ( !end_of_statement_possible && type == T_WSNL )
			continue;
		
		// Return the offset of the first token we didn't skip
		return offset-1;
	}
	
	// We're either beyond the last token or found no filtered token beyond pos
	return -1;
}

static token_p try_full_args(parser_p parser, token_type_t type, bool end_of_statement_possible) {
	list_append(&parser->tried_token_types, type);
	
	int offset = next_filtered_token_at(parser, parser->pos, end_of_statement_possible);
	if (offset == -1)
		return NULL;
	
	token_p token = &parser->module->tokens.ptr[parser->pos + offset];
	if (token->type != type)
		return NULL;
	return token;
}

static token_p consume(parser_p parser, token_p token) {
	if (token == NULL) {
		fprintf(stderr, "consume(): Tried to consume NULL!\n");
		abort();
	}
	
	size_t index = token - parser->module->tokens.ptr;
	if ( index < parser->module->tokens.len ) {
		fprintf(stderr, "consume(): Token not part of the currently parsed module!\n");
		abort();
	}
	
	// Clear tried token list and advance position
	list_destroy(&parser->tried_token_types);
	parser->pos = index + 1;
	
	return token;
}

static token_p consume_type_full_args(parser_p parser, token_type_t type, bool end_of_statement_possible) {
	token_p token = try_full_args(parser, type, end_of_statement_possible);
	if (!token)
		parser_error(parser, NULL);
	return consume(parser, token);
}

#define try(parser, type)      try_full_args((parser), (type), false)
#define try_eos(parser, type)  try_full_args((parser), (type), true)
#define consume_type(parser, type)      consume_type_full_args((parser), (type), false)
#define consume_type_eos(parser, type)  consume_type_full_args((parser), (type), true)



#if 0

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
		current_token = &parser->module->tokens.ptr[parser->module->tokens.len - 1];
	} else {
		parser->pos += offset;
		current_token = &parser->module->tokens.ptr[parser->pos];
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
		current_token = &parser->module->tokens.ptr[parser->module->tokens.len - 1];
	else
		current_token = &parser->module->tokens.ptr[parser->pos + offset];
	
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
	
	token_p current_token = &parser->module->tokens.ptr[parser->pos-1];
	
	printf("%s:%d consume_type, expected %s, got ", caller, line, token_type_name(type));
	token_print(stdout, current_token, TP_INLINE_DUMP);
	printf("\n");
	
	abort();
}

#endif


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
	return node;
}



//
// Parser rules
//

node_p parse_module(parser_p parser) {
	printf("parse_module\n");
	while ( ! try(parser, T_EOF) ) {
		if ( try(parser, T_FUNC) ) {
			printf("got func!\n");
		} else if ( try(parser, T_AND) ) {
			printf("got and!\n");
		} else {
			parser_error(parser, "You can only define functions, types or meta code in a module");
			return NULL;
		}
	}
	
	return NULL;
}
