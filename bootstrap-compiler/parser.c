#include "common.h"


//
// Parser state and utility stuff
//

struct parser_s {
	module_p module;
	size_t   pos;
	bool     at_possible_eos;
	
	list_t(token_type_t) tried_token_types;
	FILE* error_stream;
};


static void at_possible_eos(parser_p parser) {
	parser->at_possible_eos = true;
}

static token_p next_filtered_token(parser_p parser) {
	size_t pos = parser->pos;
	while (pos < parser->module->tokens.len) {
		token_p token = &parser->module->tokens.ptr[pos];
		pos++;
		
		// Skip whitespace and comment tokens
		if ( token->type == T_WS || token->type == T_COMMENT )
			continue;
		// Also skip whitespaces with newlines if we're not at a possible end
		// of statement.
		if ( token->type == T_WSNL && !parser->at_possible_eos )
			continue;
		
		// Return the first token we didn't skip
		return token;
	}
	
	// We're either beyond the last token or found no filtered token beyond pos
	return NULL;
}

static void parser_error(parser_p parser, const char* message) {
	token_p token = next_filtered_token(parser);
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
	
	fputs(" got ", parser->error_stream);
	token_print(parser->error_stream, token, TP_INLINE_DUMP);
	fputs("\n", parser->error_stream);
	
	size_t token_index = token - parser->module->tokens.ptr;
	token_print_range(parser->error_stream, parser->module, token_index, 1);
}

static token_p try(parser_p parser, token_type_t type) {
	list_append(&parser->tried_token_types, type);
	
	token_p token = next_filtered_token(parser);
	if (token && token->type == type)
		return token;
	return NULL;
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
	
	// Advance parser position and clear eos flag and tried token types
	parser->pos = index + 1;
	parser->at_possible_eos = false;
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
