// For open_memstream
#define _GNU_SOURCE

//#include <string.h>
#include "../common.h"

#define SLIM_TEST_IMPLEMENTATION
#define ST_MAX_MESSAGE_SIZE 4096
#include "slim_test.h"


struct { parser_rule_func_t rule; char* code; char* expected_ast_dump; } samples[] = {
	//
	// cexpr: literals
	//
	{ parse_expr, "foo",
		"id: \"foo\"\n"
	},
	{ parse_expr, "17",
		"intl: 17\n"
	},
	{ parse_expr, "\"str\"",
		"strl: \"str\"\n"
	},
	
	{ parse_expr, "(17)",
		"intl: 17\n"
	},
	{ parse_expr, " \n ( \n 17 \n ) \n ",
		"intl: 17\n"
	},
	//{ parse_expr, "(17", NULL },
	
	//
	// cexpr: unary operators
	//
	{ parse_expr, "+17",
		"unary_op: \"plus\"\n"
		"  op: id: \"+\"\n"
		"  arg: intl: 17\n"
	},
	{ parse_expr, "-17",
		"unary_op: \"minus\"\n"
		"  op: id: \"-\"\n"
		"  arg: intl: 17\n"
	},
	{ parse_expr, "!foo",
		"unary_op: \"not\"\n"
		"  op: id: \"!\"\n"
		"  arg: id: \"foo\"\n"
	},
	{ parse_expr, "~foo",
		"unary_op: \"compl\"\n"
		"  op: id: \"~\"\n"
		"  arg: id: \"foo\"\n"
	},
	{ parse_expr, " \n ~ \n foo \n ",
		"unary_op: \"compl\"\n"
		"  op: id: \"~\"\n"
		"  arg: id: \"foo\"\n"
	},
	
	//
	// cexpr: function calls with parenthesis
	//
	{ parse_expr, "foo(0, \"bar\", x)",
		"call: \n"
		"  target_expr: id: \"foo\"\n"
		"  args[0]: intl: 0\n"
		"  args[1]: strl: \"bar\"\n"
		"  args[2]: id: \"x\"\n"
	},
	{ parse_expr, "foo()",
		"call: \n"
		"  target_expr: id: \"foo\"\n"
	},
	{ parse_expr, "foo(1)(2)",
		"call: \n"
		"  target_expr: call: \n"
		"    target_expr: id: \"foo\"\n"
		"    args[0]: intl: 1\n"
		"  args[0]: intl: 2\n"
	},
	
	//
	// cexpr: index access
	//
	{ parse_expr, "foo[0, \"bar\", x]",
		"index: \n"
		"  target_expr: id: \"foo\"\n"
		"  args[0]: intl: 0\n"
		"  args[1]: strl: \"bar\"\n"
		"  args[2]: id: \"x\"\n"
	},
	{ parse_expr, "foo[]",
		"index: \n"
		"  target_expr: id: \"foo\"\n"
	},
	{ parse_expr, "foo[1](2)",
		"call: \n"
		"  target_expr: index: \n"
		"    target_expr: id: \"foo\"\n"
		"    args[0]: intl: 1\n"
		"  args[0]: intl: 2\n"
	},
	
	//
	// cexpr: member access
	//
	{ parse_expr, "foo.bar",
		"member: \n"
		"  aggregate: id: \"foo\"\n"
		"  member: \"bar\"\n"
	},
	{ parse_expr, "foo().bar[].x",
		"member: \n"
		"  aggregate: index: \n"
		"    target_expr: member: \n"
		"      aggregate: call: \n"
		"        target_expr: id: \"foo\"\n"
		"      member: \"bar\"\n"
		"  member: \"x\"\n"
	},
	
	//
	// expr: binary operators
	//
	{ parse_expr, "foo + bar",
		"uops: \n"
		"  list[0]: id: \"foo\"\n"
		"  list[1]: id: \"add\"\n"
		"  list[2]: id: \"bar\"\n"
	},
	{ parse_expr, "foo * bar",
		"uops: \n"
		"  list[0]: id: \"foo\"\n"
		"  list[1]: id: \"mul\"\n"
		"  list[2]: id: \"bar\"\n"
	},
	{ parse_expr, "foo ^= bar",
		"uops: \n"
		"  list[0]: id: \"foo\"\n"
		"  list[1]: id: \"bit_xor_assign\"\n"
		"  list[2]: id: \"bar\"\n"
	},
	{ parse_expr, "a + b * c = x <<= y",
		"uops: \n"
		"  list[0]: id: \"a\"\n"
		"  list[1]: id: \"add\"\n"
		"  list[2]: id: \"b\"\n"
		"  list[3]: id: \"mul\"\n"
		"  list[4]: id: \"c\"\n"
		"  list[5]: id: \"assign\"\n"
		"  list[6]: id: \"x\"\n"
		"  list[7]: id: \"shift_left_assign\"\n"
		"  list[8]: id: \"y\"\n"
	},
	{ parse_expr, "foo.x[0].y ^= bar(z.a)",
		"uops: \n"
		"  list[0]: member: \n"
		"    aggregate: index: \n"
		"      target_expr: member: \n"
		"        aggregate: id: \"foo\"\n"
		"        member: \"x\"\n"
		"      args[0]: intl: 0\n"
		"    member: \"y\"\n"
		"  list[1]: id: \"bit_xor_assign\"\n"
		"  list[2]: call: \n"
		"    target_expr: id: \"bar\"\n"
		"    args[0]: member: \n"
		"      aggregate: id: \"z\"\n"
		"      member: \"a\"\n"
	},
	
	//
	// Statements: simple cexpr as statement
	// This is used as placeholder statements for the next samples.
	//
	{ parse_stmt, "dec(x)",
		"call: \n"
		"  target_expr: id: \"dec\"\n"
		"  args[0]: id: \"x\"\n"
	},
	
	//
	// Statements: scope
	//
	{ parse_stmt, "{ x }",
		"scope: \n"
		"  stmts[0]: id: \"x\"\n"
	},
	{ parse_stmt, "do x end",
		"scope: \n"
		"  stmts[0]: id: \"x\"\n"
	},
	
	//
	// Statements: while
	//
	{ parse_stmt, "while x > 0 do dec(x) end",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  body[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"x\"\n"
	},
	{ parse_stmt, "while x > 0 { dec(x) }",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  body[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"x\"\n"
	},
	{ parse_stmt, "while x > 0 do \n dec(x) \n end",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  body[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"x\"\n"
	},
	{ parse_stmt, "while x > 0 { \n dec(x) \n }",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  body[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"x\"\n"
	},
	{ parse_stmt, "while x > 0 \n do \n dec(x) \n end",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  body[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"x\"\n"
	},
	{ parse_stmt, "while x > 0 \n { \n dec(x) \n }",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  body[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"x\"\n"
	},
	{ parse_stmt, "while x > 0 \n dec(x) \n end",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  body[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"x\"\n"
	},
	
	//
	// Statements: if
	//
	{ parse_stmt, "if x > 0 do dec(x) end",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"x\"\n"
	},
	{ parse_stmt, "if x > 0 do dec(x) else inc(x) end",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"x\"\n"
		"  false_case[0]: call: \n"
		"    target_expr: id: \"inc\"\n"
		"    args[0]: id: \"x\"\n"
	},
	{ parse_stmt, "if x > 0 { dec(x) }",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"x\"\n"
	},
	{ parse_stmt, "if x > 0 { dec(x) } else { inc(x) }",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"x\"\n"
		"  false_case[0]: call: \n"
		"    target_expr: id: \"inc\"\n"
		"    args[0]: id: \"x\"\n"
	},
	// With ignored line breaks
	{ parse_stmt, "if x > 0 \n do \n dec(x) \n end",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"x\"\n"
	},
	{ parse_stmt, "if x > 0 \n do \n dec(x) \n else \n inc(x) \n end",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"x\"\n"
		"  false_case[0]: call: \n"
		"    target_expr: id: \"inc\"\n"
		"    args[0]: id: \"x\"\n"
	},
	{ parse_stmt, "if x > 0 \n { \n dec(x) \n }",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"x\"\n"
	},
	{ parse_stmt, "if x > 0 \n { \n dec(x) \n } \n else \n { \n inc(x) \n }",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"x\"\n"
		"  false_case[0]: call: \n"
		"    target_expr: id: \"inc\"\n"
		"    args[0]: id: \"x\"\n"
	},
	// With optional "do"
	{ parse_stmt, "if x > 0 \n dec(x) \n end",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"x\"\n"
	},
	{ parse_stmt, "if x > 0 \n dec(x) \n else \n inc(x) \n end",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"x\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"x\"\n"
		"  false_case[0]: call: \n"
		"    target_expr: id: \"inc\"\n"
		"    args[0]: id: \"x\"\n"
	},
	
	//
	// Statements: var definitions
	//
	{ parse_stmt, "int x",
		"var: \n"
		"  type_expr: id: \"int\"\n"
		"  bindings[0]: binding: \"x\"\n"
		"    value: \n"
	},
	{ parse_stmt, "int x = 7",
		"var: \n"
		"  type_expr: id: \"int\"\n"
		"  bindings[0]: binding: \"x\"\n"
		"    value: intl: 7\n"
	},
	{ parse_stmt, "int x, y, z",
		"var: \n"
		"  type_expr: id: \"int\"\n"
		"  bindings[0]: binding: \"x\"\n"
		"    value: \n"
		"  bindings[1]: binding: \"y\"\n"
		"    value: \n"
		"  bindings[2]: binding: \"z\"\n"
		"    value: \n"
	},
	{ parse_stmt, "int x = 1, y = 2, z = 3",
		"var: \n"
		"  type_expr: id: \"int\"\n"
		"  bindings[0]: binding: \"x\"\n"
		"    value: intl: 1\n"
		"  bindings[1]: binding: \"y\"\n"
		"    value: intl: 2\n"
		"  bindings[2]: binding: \"z\"\n"
		"    value: intl: 3\n"
	},
	{ parse_stmt, "int x = 1, y, z = 3",
		"var: \n"
		"  type_expr: id: \"int\"\n"
		"  bindings[0]: binding: \"x\"\n"
		"    value: intl: 1\n"
		"  bindings[1]: binding: \"y\"\n"
		"    value: \n"
		"  bindings[2]: binding: \"z\"\n"
		"    value: intl: 3\n"
	},
	{ parse_stmt, "int x, y = foo(1 + 2), z",
		"var: \n"
		"  type_expr: id: \"int\"\n"
		"  bindings[0]: binding: \"x\"\n"
		"    value: \n"
		"  bindings[1]: binding: \"y\"\n"
		"    value: call: \n"
		"      target_expr: id: \"foo\"\n"
		"      args[0]: uops: \n"
		"        list[0]: intl: 1\n"
		"        list[1]: id: \"add\"\n"
		"        list[2]: intl: 2\n"
		"  bindings[2]: binding: \"z\"\n"
		"    value: \n"
	},
	{ parse_stmt, "ref(int)[] x",
		"var: \n"
		"  type_expr: index: \n"
		"    target_expr: call: \n"
		"      target_expr: id: \"ref\"\n"
		"      args[0]: id: \"int\"\n"
		"  bindings[0]: binding: \"x\"\n"
		"    value: \n"
	},
	
	//
	// Statements: binary operators
	//
	{ parse_stmt, "x + y",
		"uops: \n"
		"  list[0]: id: \"x\"\n"
		"  list[1]: id: \"add\"\n"
		"  list[2]: id: \"y\"\n"
	},
	{ parse_stmt, "x + y ;",
		"uops: \n"
		"  list[0]: id: \"x\"\n"
		"  list[1]: id: \"add\"\n"
		"  list[2]: id: \"y\"\n"
	},
	{ parse_stmt, "x + y * z",
		"uops: \n"
		"  list[0]: id: \"x\"\n"
		"  list[1]: id: \"add\"\n"
		"  list[2]: id: \"y\"\n"
		"  list[3]: id: \"mul\"\n"
		"  list[4]: id: \"z\"\n"
	},
	{ parse_stmt, "x corss y dot z",
		"uops: \n"
		"  list[0]: id: \"x\"\n"
		"  list[1]: id: \"corss\"\n"
		"  list[2]: id: \"y\"\n"
		"  list[3]: id: \"dot\"\n"
		"  list[4]: id: \"z\"\n"
	},
	
	//
	// Statements: while and if modifier
	//
	{ parse_stmt, "dec(a) while a > 0",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"a\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  body[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"a\"\n"
	},
	{ parse_stmt, "dec(a) while a > 0 ;",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"a\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  body[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"a\"\n"
	},
	{ parse_stmt, "a -= 1 while a > 0",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"a\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  body[0]: uops: \n"
		"    list[0]: id: \"a\"\n"
		"    list[1]: id: \"sub_assign\"\n"
		"    list[2]: intl: 1\n"
	},
	{ parse_stmt, "dec(a) while a > 0 and x",
		"while_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"a\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"    list[3]: id: \"and\"\n"
		"    list[4]: id: \"x\"\n"
		"  body[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"a\"\n"
	},
	{ parse_stmt, "dec(a) if a > 0",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"a\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"a\"\n"
	},
	{ parse_stmt, "dec(a) if a > 0 ;",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"a\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"a\"\n"
	},
	{ parse_stmt, "a -= 1 if a > 0",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"a\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"  true_case[0]: uops: \n"
		"    list[0]: id: \"a\"\n"
		"    list[1]: id: \"sub_assign\"\n"
		"    list[2]: intl: 1\n"
	},
	{ parse_stmt, "dec(a) if a > 0 and x",
		"if_stmt: \n"
		"  cond: uops: \n"
		"    list[0]: id: \"a\"\n"
		"    list[1]: id: \"gt\"\n"
		"    list[2]: intl: 0\n"
		"    list[3]: id: \"and\"\n"
		"    list[4]: id: \"x\"\n"
		"  true_case[0]: call: \n"
		"    target_expr: id: \"dec\"\n"
		"    args[0]: id: \"a\"\n"
  	},
  	
  	//
  	// Statements: return
  	//
	{ parse_stmt, "return",
		"return_stmt: \n"
  	},
	{ parse_stmt, "return x",
		"return_stmt: \n"
		"  args[0]: id: \"x\"\n"
  	},
	{ parse_stmt, "return x, y, z",
		"return_stmt: \n"
		"  args[0]: id: \"x\"\n"
		"  args[1]: id: \"y\"\n"
		"  args[2]: id: \"z\"\n"
  	},
	{ parse_stmt, "return dec(x) + 7, foo.bar(1, 2, 3)",
		"return_stmt: \n"
		"  args[0]: uops: \n"
		"    list[0]: call: \n"
		"      target_expr: id: \"dec\"\n"
		"      args[0]: id: \"x\"\n"
		"    list[1]: id: \"add\"\n"
		"    list[2]: intl: 7\n"
		"  args[1]: call: \n"
		"    target_expr: member: \n"
		"      aggregate: id: \"foo\"\n"
		"      member: \"bar\"\n"
		"    args[0]: intl: 1\n"
		"    args[1]: intl: 2\n"
		"    args[2]: intl: 3\n"
  	},
  	
  	//
  	// Definition: functions
  	//
	{ parse_module, "func foo in(int argc, int.ptr argv) out(int) { int a; a += 1 if argc < 2; { foo(a) } }",
		"module: \"\"\n"
		"  defs[0]: func: \"foo\"\n"
		"    in[0]: arg: \"argc\"\n"
		"      type_expr: id: \"int\"\n"
		"    in[1]: arg: \"argv\"\n"
		"      type_expr: member: \n"
		"        aggregate: id: \"int\"\n"
		"        member: \"ptr\"\n"
		"    out[0]: arg: \"\"\n"
		"      type_expr: id: \"int\"\n"
		"    body[0]: var: \n"
		"      type_expr: id: \"int\"\n"
		"      bindings[0]: binding: \"a\"\n"
		"        value: \n"
		"    body[1]: if_stmt: \n"
		"      cond: uops: \n"
		"        list[0]: id: \"argc\"\n"
		"        list[1]: id: \"lt\"\n"
		"        list[2]: intl: 2\n"
		"      true_case[0]: uops: \n"
		"        list[0]: id: \"a\"\n"
		"        list[1]: id: \"add_assign\"\n"
		"        list[2]: intl: 1\n"
		"    body[2]: scope: \n"
		"      stmts[0]: call: \n"
		"        target_expr: id: \"foo\"\n"
		"        args[0]: id: \"a\"\n"
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
			node_print(node, P_PARSER, output);
		fclose(output);
		
		st_check_str(output_ptr, samples[i].expected_ast_dump);
		
		list_destroy(&module->tokens);
	}
	
	free(output_ptr);
}


struct { char* code; char* ast_dump; } statement_pool[] = {
	{	"{ dec(x) } \n",
		"    body[%d]: scope: \n"
		"      stmts[0]: call: \n"
		"        target_expr: id: \"dec\"\n"
		"        args[0]: id: \"x\"\n"
	},
	{	"do dec(x) end \n",
		"    body[%d]: scope: \n"
		"      stmts[0]: call: \n"
		"        target_expr: id: \"dec\"\n"
		"        args[0]: id: \"x\"\n"
	},
	{	"while x > 0 \n dec(x) \n end \n",
		"    body[%d]: while_stmt: \n"
		"      cond: uops: \n"
		"        list[0]: id: \"x\"\n"
		"        list[1]: id: \"gt\"\n"
		"        list[2]: intl: 0\n"
		"      body[0]: call: \n"
		"        target_expr: id: \"dec\"\n"
		"        args[0]: id: \"x\"\n"
	},
	{	"if x > 0 \n dec(x) \n else inc(x) end \n",
		"    body[%d]: if_stmt: \n"
		"      cond: uops: \n"
		"        list[0]: id: \"x\"\n"
		"        list[1]: id: \"gt\"\n"
		"        list[2]: intl: 0\n"
		"      true_case[0]: call: \n"
		"        target_expr: id: \"dec\"\n"
		"        args[0]: id: \"x\"\n"
		"      false_case[0]: call: \n"
		"        target_expr: id: \"inc\"\n"
		"        args[0]: id: \"x\"\n"
	},
	{	"return x + 1 \n",
		"    body[%d]: return_stmt: \n"
		"      args[0]: uops: \n"
		"        list[0]: id: \"x\"\n"
		"        list[1]: id: \"add\"\n"
		"        list[2]: intl: 1\n"
	},
	{	"int x, y = 1 \n",
		"    body[%d]: var: \n"
		"      type_expr: id: \"int\"\n"
		"      bindings[0]: binding: \"x\"\n"
		"        value: \n"
		"      bindings[1]: binding: \"y\"\n"
		"        value: intl: 1\n"
	},
	{	"x + y \n",
		"    body[%d]: uops: \n"
		"      list[0]: id: \"x\"\n"
		"      list[1]: id: \"add\"\n"
		"      list[2]: id: \"y\"\n"
	},
};

void test_statement_combinations() {
	char   *code_ptr = NULL, *ast_dump_ptr = NULL, *output_ptr = NULL;
	size_t  code_len = 0,     ast_dump_len = 0,     output_len = 0;
	FILE   *code     = NULL, *ast_dump     = NULL, *output     = NULL;
	
	const size_t sample_count = sizeof(statement_pool) / sizeof(statement_pool[0]);
	for(size_t i = 0; i < sample_count; i++) {
		for(size_t j = 0; j < sample_count; j++) {
			for(size_t k = 0; k < sample_count; k++) {
				code = open_memstream(&code_ptr, &code_len);
					fputs("func main do\n", code);
					fputs(statement_pool[i].code, code);
					fputs(statement_pool[j].code, code);
					fputs(statement_pool[k].code, code);
					fputs("end\n", code);
				fclose(code);
				
				ast_dump = open_memstream(&ast_dump_ptr, &ast_dump_len);
					fprintf(ast_dump,
						"module: \"\"\n"
						"  defs[0]: func: \"main\"\n"
					);
					fprintf(ast_dump, statement_pool[i].ast_dump, 0);
					fprintf(ast_dump, statement_pool[j].ast_dump, 1);
					fprintf(ast_dump, statement_pool[k].ast_dump, 2);
				fclose(ast_dump);
				
				
				//printf("sample code i = %zu, j = %zu, k = %zu:\n%s\n", i, j, k, code_ptr);
				module_p module = &(module_t){
					.filename = "parser_test.c/test_statement_combinations",
					.source   = str_from_c(code_ptr)
				};
				
				size_t errors = tokenize(module->source, &module->tokens, stderr);
				st_check_int(errors, 0);
				
				output = open_memstream(&output_ptr, &output_len);
					node_p node = parse(module, parse_module, stderr);
					node_print(node, P_PARSER, output);
				fclose(output);
				
				st_check_str(output_ptr, ast_dump_ptr);
				
				list_destroy(&module->tokens);
				free(code_ptr);  free(ast_dump_ptr);  free(output_ptr);
				code_ptr = NULL; ast_dump_ptr = NULL; output_ptr = NULL;
				code_len = 0;    ast_dump_len = 0;    output_len = 0;
			}
		}
	}
}

int main() {
	st_run(test_samples);
	st_run(test_statement_combinations);
	return st_show_report();
}