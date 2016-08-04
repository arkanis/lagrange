// For open_memstream
#define _GNU_SOURCE

//#include <string.h>
#include "../common.h"

#define SLIM_TEST_IMPLEMENTATION
#include "slim_test.h"


struct { char* code; char* expected_ast_dump; } samples[] = {
	{ "x + y",
		"op: \n"
		"  def: op_buildin: \"add\"\n"
		"  a: id: \"x\"\n"
		"  id: id: \"add\"\n"
		"  b: id: \"y\"\n"
	},
	{ "a + b * c",
		"op: \n"
		"  def: op_buildin: \"add\"\n"
		"  a: id: \"a\"\n"
		"  id: id: \"add\"\n"
		"  b: op: \n"
		"    def: op_buildin: \"mul\"\n"
		"    a: id: \"b\"\n"
		"    id: id: \"mul\"\n"
		"    b: id: \"c\"\n"
	},
	{ "1 * 2 + 3 * 4",
		"op: \n"
		"  def: op_buildin: \"add\"\n"
		"  a: op: \n"
		"    def: op_buildin: \"mul\"\n"
		"    a: intl: 1\n"
		"    id: id: \"mul\"\n"
		"    b: intl: 2\n"
		"  id: id: \"add\"\n"
		"  b: op: \n"
		"    def: op_buildin: \"mul\"\n"
		"    a: intl: 3\n"
		"    id: id: \"mul\"\n"
		"    b: intl: 4\n"
	},
};

void test_samples() {
	char*  output_ptr = NULL;
	size_t output_len = 0;
	FILE*  output = NULL;
	
	for(size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
		module_p module = &(module_t){
			.filename = "resolve_uops_test.c/test_samples",
			.source   = str_from_c(samples[i].code)
		};
		
		// Initialize buildin stuff
		node_p buildins = node_alloc(NT_MODULE);
		buildins->name = str_from_c("buildins");
			add_buildin_ops_to_namespace(buildins);
		fill_namespaces(NULL, buildins, NULL);
		
		size_t errors = tokenize(module->source, &module->tokens, stderr);
		st_check_int(errors, 0);
		
		node_p node = parse(module, parse_expr, stderr);
		node_append(buildins, &buildins->module.defs, node);
		node = pass_resolve_uops(module, node);
		
		output = open_memstream(&output_ptr, &output_len);
			node_print(node, P_PARSER, P_PARSER, output);
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