// for getopt()
#define _GNU_SOURCE
#include <unistd.h>
#include "common.h"


int main(int argc, char** argv) {
	// Process command line arguments
	bool show_tokens = false, show_parser_ast = false, show_filled_namespaces = false;
	bool show_resloved_uops = false;
	int opt;
	while ( (opt = getopt(argc, argv, "tpno")) != -1 ) {
		switch (opt) {
			case 't': show_tokens = true;            break;
			case 'p': show_parser_ast = true;        break;
			case 'n': show_filled_namespaces = true; break;
			case 'o': show_resloved_uops = true;     break;
			default:
				fprintf(stderr, "usage: %s [ -tp ] source-file\n", argv[0]);
				return 1;
		}
	}
	
	if ( !(optind + 1 == argc) ) {
		fprintf(stderr, "usage: %s [ -tp ] source-file\n", argv[0]);
		return 1;
	}
	
	char* source_file = argv[optind];
	
	
	// Initialize module
	module_p module = &(module_t){
		.filename = source_file,
		.source   = str_fload(source_file)
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
	if (show_tokens) {
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
	}
	
	
	// Step 2 - Parse tokens into an AST
	node_p node = parse(module, parse_module, stderr);
	if (show_parser_ast)
		node_print(node, P_PARSER, stdout);
	
	// Step 3 - Fill namespaces
	fill_namespaces(module, node, NULL);
	if (show_filled_namespaces)
		node_print(node, P_NAMESPACE, stdout);
	
	// Step 4 - Resolve uops nodes
	node = pass_resolve_uops(module, node);
	if (show_resloved_uops)
		node_print(node, P_PARSER, stdout);
	
	cleanup_tokenizer:
		list_destroy(&module->tokens);
		str_free(&module->source);
	return exit_code;
}