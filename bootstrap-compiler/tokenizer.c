#include "common.h"


typedef struct {
	str_t        source;
	size_t       pos;
	token_list_p tokens;
	FILE*        error_stream;
	size_t       error_count;
} tokenizer_ctx_t, *tokenizer_ctx_p;

static bool next_token(tokenizer_ctx_p ctx);


size_t tokenize(str_t source, token_list_p tokens, FILE* error_stream) {
	tokenizer_ctx_t ctx = (tokenizer_ctx_t){
		.source = source,
		.pos    = 0,
		.tokens = tokens,
		.error_stream = error_stream,
		.error_count  = 0
	};
	
	while ( next_token(&ctx) ) { }
	
	return ctx.error_count;
}

int token_line(module_p module, token_p token) {
	int line = 1;
	for(char* c = token->source.ptr; c >= module->source.ptr; c--) {
		if (*c == '\n')
			line++;
	}
	return line;
}

int token_col(module_p module, token_p token) {
	int col = 0;
	for(char* c = token->source.ptr; *c != '\n' && c >= module->source.ptr; c--)
		col++;
	return col;
}