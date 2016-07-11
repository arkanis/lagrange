#include "common.h"

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "usage: %s source-file\n", argv[0]);
		return 1;
	}
	
	module_p module = &(module_t){
		.filename = argv[1],
		.source   = str_fload(argv[1])
	};
	int exit_code = 0;
	
	// Step 1 - Tokenize source
	size_t error_count = 0;
	if ( (error_count = tokenize(module->source, &module->tokens, stderr)) > 0 ) {
		// Just output errors and exit
		for(size_t i = 0; i < module->tokens.len; i++) {
			token_p t = &module->tokens.ptr[i];
			if (t->type == T_ERROR) {
				fprintf(stderr, "%s:%d:%d: %s\n",
					module->filename, token_line(module, t), token_col(module, t),
					t->str_val.ptr
				);
				token_print_line(stderr, module, t);
			}
		}
		
		exit_code = 1;
		goto cleanup_tokenizer;
	}
	
	// Print tokens
	for(size_t i = 0; i < module->tokens.len; i++) {
		token_p t = &module->tokens.ptr[i];
		if (t->type == T_COMMENT || t->type == T_WS) {
			token_print(stdout, t, TP_SOURCE);
		} else if (t->type == T_WSNL || t->type == T_EOF) {
			printf(" ");
			token_print(stdout, t, TP_DUMP);
			printf(" ");
		} else {
			token_print(stdout, t, TP_DUMP);
		}
	}
	printf("\n");
	
	// Step 2 - Parse tokens into an AST
	node_p node = parse(module, parse_expr, stderr);
	node_print(node, stdout);
	
	
	cleanup_tokenizer:
		list_destroy(&module->tokens);
		str_free(&module->source);
	return exit_code;
}