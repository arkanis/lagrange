#include "common.h"


//
// Parser state and utility stuff
//

struct parser_s {
	module_p module;
	size_t pos;
};


int next_filtered_token_at(parser_p parser, size_t pos, bool ignore_ws_eos) {
	size_t offset = 0;
	while (pos + offset < (size_t)parser->module->tokens.len) {
		token_type_t type = parser->module->tokens.ptr[pos + offset].type;
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