// For open_memstream
#define _GNU_SOURCE

//#include <string.h>
#include "../common.h"

#define SLIM_TEST_IMPLEMENTATION
#include "slim_test.h"


struct { parser_rule_func_t rule; char* code; char* expected_ast_dump; } samples[] = {
	{ parse_expr, "foo",
		"id: \"foo\"\n"
		"  type: \n"
	},
	{ parse_expr, "17",
		"intl: 17\n"
		"  type: \n"
	},
	{ parse_expr, "\"str\"",
		"strl: \"str\"\n"
		"  type: \n"
	},
};

void test_samples() {
	char*  output_ptr = NULL;
	size_t output_len = 0;
	FILE*  output = NULL;
	
	for(size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
		module_p module = &(module_t){
			.filename = "parser_test.c/test_samples",
			.source   = str_from_c(samples[i].code)
		};
		
		size_t errors = tokenize(module->source, &module->tokens, stderr);
		st_check_int(errors, 0);
		
		output = open_memstream(&output_ptr, &output_len);
			node_p node = parse(module, samples[i].rule, stderr);
			node_print(node, output);
		fclose(output);
		
		st_check_str(output_ptr, samples[i].expected_ast_dump);
		
		list_destroy(&module->tokens);
	}
	
	free(output_ptr);
}

int main() {
	st_run(test_samples);
	return st_show_report();
}