#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"

int main(int argc, char** argv) {
	if (argc > 2) {
		fprintf(stderr, "usage: %s [source-file]\n", argv[0]);
		return 1;
	}
	
	FILE* input = stdin;
	if (argc == 2) {
		input = fopen(argv[1], "rb");
		if (input == NULL) {
			perror("fopen()");
			return 1;
		}
	}
	
	tok_init();
	token_p tokens = NULL;
	size_t token_count = 0;
	
	token_t t;
	do {
		t = tok_next(input, stderr);
		if (t.type == T_COMMENT || t.type == T_WS) {
			printf("%.*s", (int)t.length, t.str_val);
		} else {
			char buffer[128];
			tok_dump(t, buffer, sizeof(buffer));
			printf("%s ", buffer);
		}
		
		token_count++;
		tokens = realloc(tokens, sizeof(token_t) * token_count);
		tokens[token_count-1] = t;
	} while (t.type != T_EOF);
	printf("\n");
	
	if (input != stdin)
		fclose(input);
	
	parse_module(tokens, token_count);
	
	for(size_t i = 0; i < token_count; i++)
		tok_free(tokens[i]);
	
	return 0;
}