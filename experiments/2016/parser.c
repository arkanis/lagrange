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


int next_filtered_token_at(parser_p parser, size_t pos) {
	size_t offset = 0;
	while (pos + offset < parser->token_count) {
		token_type_t type = parser->tokens[pos + offset].type;
		// Return the offset if we found a filtered token there
		if ( type != T_WS && type != T_COMMENT )
			return offset;
		offset++;
	}
	
	// We're either beyond the last token or found no filtered token beyond pos
	return -1;
}

#define consume(parser) consume_impl((parser), __FUNCTION__, __LINE__)
token_p consume_impl(parser_p parser, const char* caller, int line) {
	int offset = next_filtered_token_at(parser, parser->pos);
	// Return NULL if there are no more filtered tokens
	if (offset == -1)
		return NULL;
	
	parser->pos += offset;
	token_p current_token = &parser->tokens[parser->pos];
	parser->pos++;
	
	char token_desc[128];
	token_dump(current_token, token_desc, sizeof(token_desc));
	printf("%s:%d consume %s\n", caller, line, token_desc);
	token_annotate_line(current_token);
	
	return current_token;
}

//
// Definitions
//

tree_p parse_module(tokenized_file_p file) {
	parser_t parser_storage = { file->tokens, file->token_count, 0 };
	parser_p parser = &parser_storage;
	
	token_p t;
	do {
		t = consume(parser);
	} while (t->type != T_EOF);
	
	return NULL;
}