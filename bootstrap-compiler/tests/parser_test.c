// For open_memstream
#define _GNU_SOURCE

//#include <string.h>
#include "../common.h"

#define SLIM_TEST_IMPLEMENTATION
#include "slim_test.h"


struct { parser_rule_func_t rule; char* code; char* expected_ast_dump; } samples[] = {
	//
	// cexpr: literals
	//
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
	
	{ parse_expr, "(17)",
		"intl: 17\n"
		"  type: \n"
	},
	{ parse_expr, " \n ( \n 17 \n ) \n ",
		"intl: 17\n"
		"  type: \n"
	},
	//{ parse_expr, "(17", NULL },
	
	//
	// cexpr: unary operators
	//
	{ parse_expr, "+17",
		"unary_op: 0\n"
		"  arg: intl: 17\n"
		"    type: \n"
		"  type: \n"
	},
	{ parse_expr, "-17",
		"unary_op: 1\n"
		"  arg: intl: 17\n"
		"    type: \n"
		"  type: \n"
	},
	{ parse_expr, "!foo",
		"unary_op: 2\n"
		"  arg: id: \"foo\"\n"
		"    type: \n"
		"  type: \n"
	},
	{ parse_expr, "~foo",
		"unary_op: 3\n"
		"  arg: id: \"foo\"\n"
		"    type: \n"
		"  type: \n"
	},
	{ parse_expr, " \n ~ \n foo \n ",
		"unary_op: 3\n"
		"  arg: id: \"foo\"\n"
		"    type: \n"
		"  type: \n"
	},
	
	//
	// cexpr: function calls with parenthesis
	//
	{ parse_expr, "foo(0, \"bar\", x)",
		"call: \n"
		"  target_expr: id: \"foo\"\n"
		"    type: \n"
		"  args[0]: intl: 0\n"
		"    type: \n"
		"  args[1]: strl: \"bar\"\n"
		"    type: \n"
		"  args[2]: id: \"x\"\n"
		"    type: \n"
		"  type: \n"
	},
	{ parse_expr, "foo()",
		"call: \n"
		"  target_expr: id: \"foo\"\n"
		"    type: \n"
		"  type: \n"
	},
	{ parse_expr, "foo(1)(2)",
		"call: \n"
		"  target_expr: call: \n"
		"    target_expr: id: \"foo\"\n"
		"      type: \n"
		"    args[0]: intl: 1\n"
		"      type: \n"
		"    type: \n"
		"  args[0]: intl: 2\n"
		"    type: \n"
		"  type: \n"
	},
	
	//
	// cexpr: index access
	//
	{ parse_expr, "foo[0, \"bar\", x]",
		"index: \n"
		"  target_expr: id: \"foo\"\n"
		"    type: \n"
		"  args[0]: intl: 0\n"
		"    type: \n"
		"  args[1]: strl: \"bar\"\n"
		"    type: \n"
		"  args[2]: id: \"x\"\n"
		"    type: \n"
		"  type: \n"
	},
	{ parse_expr, "foo[]",
		"index: \n"
		"  target_expr: id: \"foo\"\n"
		"    type: \n"
		"  type: \n"
	},
	{ parse_expr, "foo[1](2)",
		"call: \n"
		"  target_expr: index: \n"
		"    target_expr: id: \"foo\"\n"
		"      type: \n"
		"    args[0]: intl: 1\n"
		"      type: \n"
		"    type: \n"
		"  args[0]: intl: 2\n"
		"    type: \n"
		"  type: \n"
	},
	
	//
	// cexpr: member access
	//
	{ parse_expr, "foo.bar",
		"member: \n"
		"  aggregate: id: \"foo\"\n"
		"    type: \n"
		"  member: \"bar\"\n"
		"  type: \n"
	},
	{ parse_expr, "foo().bar[].x",
		"member: \n"
		"  aggregate: index: \n"
		"    target_expr: member: \n"
		"      aggregate: call: \n"
		"        target_expr: id: \"foo\"\n"
		"          type: \n"
		"        type: \n"
		"      member: \"bar\"\n"
		"      type: \n"
		"    type: \n"
		"  member: \"x\"\n"
		"  type: \n"
	},
	
	//
	// expr: binary operators
	//
	{ parse_expr, "foo + bar",
		"uops: \n"
		"  list[0]: id: \"foo\"\n"
		"    type: \n"
		"  list[1]: id: \"+\"\n"
		"    type: \n"
		"  list[2]: id: \"bar\"\n"
		"    type: \n"
	},
	{ parse_expr, "foo * bar",
		"uops: \n"
		"  list[0]: id: \"foo\"\n"
		"    type: \n"
		"  list[1]: id: \"*\"\n"
		"    type: \n"
		"  list[2]: id: \"bar\"\n"
		"    type: \n"
	},
	{ parse_expr, "foo ^= bar",
		"uops: \n"
		"  list[0]: id: \"foo\"\n"
		"    type: \n"
		"  list[1]: id: \"^=\"\n"
		"    type: \n"
		"  list[2]: id: \"bar\"\n"
		"    type: \n"
	},
	{ parse_expr, "a + b * c = x <<= y",
		"uops: \n"
		"  list[0]: id: \"a\"\n"
		"    type: \n"
		"  list[1]: id: \"+\"\n"
		"    type: \n"
		"  list[2]: id: \"b\"\n"
		"    type: \n"
		"  list[3]: id: \"*\"\n"
		"    type: \n"
		"  list[4]: id: \"c\"\n"
		"    type: \n"
		"  list[5]: id: \"=\"\n"
		"    type: \n"
		"  list[6]: id: \"x\"\n"
		"    type: \n"
		"  list[7]: id: \"<<=\"\n"
		"    type: \n"
		"  list[8]: id: \"y\"\n"
		"    type: \n"
	},
	{ parse_expr, "foo.x[0].y ^= bar(z.a)",
		"uops: \n"
		"  list[0]: member: \n"
		"    aggregate: index: \n"
		"      target_expr: member: \n"
		"        aggregate: id: \"foo\"\n"
		"          type: \n"
		"        member: \"x\"\n"
		"        type: \n"
		"      args[0]: intl: 0\n"
		"        type: \n"
		"      type: \n"
		"    member: \"y\"\n"
		"    type: \n"
		"  list[1]: id: \"^=\"\n"
		"    type: \n"
		"  list[2]: call: \n"
		"    target_expr: id: \"bar\"\n"
		"      type: \n"
		"    args[0]: member: \n"
		"      aggregate: id: \"z\"\n"
		"        type: \n"
		"      member: \"a\"\n"
		"      type: \n"
		"    type: \n"
	},
	
	//
	// Statements: scope
	//
	{ parse_stmt, "{ x }",
		"scope: \n"
		"  stmts[0]: id: \"x\"\n"
		"    type: \n"
		"  ns: \n"
	},
	{ parse_stmt, "do x end",
		"scope: \n"
		"  stmts[0]: id: \"x\"\n"
		"    type: \n"
		"  ns: \n"
	},
	
	//
	// Statements: while
	//
	{ parse_stmt, "while x > 0 do dec(x) end",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"      type: \n"
		"    list[1]: id: \">\"\n"
		"      type: \n"
		"    list[2]: intl: 0\n"
		"      type: \n"
		"  body[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
	},
	{ parse_stmt, "while x > 0 { dec(x) }",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"      type: \n"
		"    list[1]: id: \">\"\n"
		"      type: \n"
		"    list[2]: intl: 0\n"
		"      type: \n"
		"  body[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
	},
	{ parse_stmt, "while x > 0 do \n dec(x) \n end",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"      type: \n"
		"    list[1]: id: \">\"\n"
		"      type: \n"
		"    list[2]: intl: 0\n"
		"      type: \n"
		"  body[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
	},
	{ parse_stmt, "while x > 0 { \n dec(x) \n }",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"      type: \n"
		"    list[1]: id: \">\"\n"
		"      type: \n"
		"    list[2]: intl: 0\n"
		"      type: \n"
		"  body[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
	},
	{ parse_stmt, "while x > 0 \n do \n dec(x) \n end",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"      type: \n"
		"    list[1]: id: \">\"\n"
		"      type: \n"
		"    list[2]: intl: 0\n"
		"      type: \n"
		"  body[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
	},
	{ parse_stmt, "while x > 0 \n { \n dec(x) \n }",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"      type: \n"
		"    list[1]: id: \">\"\n"
		"      type: \n"
		"    list[2]: intl: 0\n"
		"      type: \n"
		"  body[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
	},
	{ parse_stmt, "while x > 0 \n dec(x) \n end",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"      type: \n"
		"    list[1]: id: \">\"\n"
		"      type: \n"
		"    list[2]: intl: 0\n"
		"      type: \n"
		"  body[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
	},
	
	//
	// Statements: if
	//
	{ parse_stmt, "if x > 0 do dec(x) end",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"      type: \n"
		"    list[1]: id: \">\"\n"
		"      type: \n"
		"    list[2]: intl: 0\n"
		"      type: \n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
		"  true_ns: \n"
	},
	{ parse_stmt, "if x > 0 do dec(x) else inc(x) end",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"      type: \n"
		"    list[1]: id: \">\"\n"
		"      type: \n"
		"    list[2]: intl: 0\n"
		"      type: \n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
		"  false_case[0]: call: \n"
		"    target_expr: id: \"inc\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
		"  true_ns: \n"
	},
	{ parse_stmt, "if x > 0 { dec(x) }",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"      type: \n"
		"    list[1]: id: \">\"\n"
		"      type: \n"
		"    list[2]: intl: 0\n"
		"      type: \n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
		"  true_ns: \n"
	},
	{ parse_stmt, "if x > 0 { dec(x) } else { inc(x) }",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"      type: \n"
		"    list[1]: id: \">\"\n"
		"      type: \n"
		"    list[2]: intl: 0\n"
		"      type: \n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
		"  false_case[0]: call: \n"
		"    target_expr: id: \"inc\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
		"  true_ns: \n"
	},
	// With ignored line breaks
	{ parse_stmt, "if x > 0 \n do \n dec(x) \n end",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"      type: \n"
		"    list[1]: id: \">\"\n"
		"      type: \n"
		"    list[2]: intl: 0\n"
		"      type: \n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
		"  true_ns: \n"
	},
	{ parse_stmt, "if x > 0 \n do \n dec(x) \n else \n inc(x) \n end",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"      type: \n"
		"    list[1]: id: \">\"\n"
		"      type: \n"
		"    list[2]: intl: 0\n"
		"      type: \n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
		"  false_case[0]: call: \n"
		"    target_expr: id: \"inc\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
		"  true_ns: \n"
	},
	{ parse_stmt, "if x > 0 \n { \n dec(x) \n }",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"      type: \n"
		"    list[1]: id: \">\"\n"
		"      type: \n"
		"    list[2]: intl: 0\n"
		"      type: \n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
		"  true_ns: \n"
	},
	{ parse_stmt, "if x > 0 \n { \n dec(x) \n } \n else \n { \n inc(x) \n }",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"      type: \n"
		"    list[1]: id: \">\"\n"
		"      type: \n"
		"    list[2]: intl: 0\n"
		"      type: \n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
		"  false_case[0]: call: \n"
		"    target_expr: id: \"inc\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
		"  true_ns: \n"
	},
	// With optional "do"
	{ parse_stmt, "if x > 0 \n dec(x) \n end",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"      type: \n"
		"    list[1]: id: \">\"\n"
		"      type: \n"
		"    list[2]: intl: 0\n"
		"      type: \n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
		"  true_ns: \n"
	},
	{ parse_stmt, "if x > 0 \n dec(x) \n else \n inc(x) \n end",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"      type: \n"
		"    list[1]: id: \">\"\n"
		"      type: \n"
		"    list[2]: intl: 0\n"
		"      type: \n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
		"  false_case[0]: call: \n"
		"    target_expr: id: \"inc\"\n"
		"      type: \n"
		"    args[0]: id: \"x\"\n"
		"      type: \n"
		"    type: \n"
		"  true_ns: \n"
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