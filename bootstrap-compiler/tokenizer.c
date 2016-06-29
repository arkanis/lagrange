#include "common.h"


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