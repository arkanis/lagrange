// For open_memstream
#define _GNU_SOURCE

#include <string.h>
#include "../common.h"

#define SLIM_TEST_IMPLEMENTATION
#include "slim_test.h"


struct { char* code; size_t tokens_len; token_p tokens_ptr; } samples[] = {
	// Empty string
	{ "", 1, (token_t[]){
		{ .type = T_EOF,    .source = { 0, "" } }
	} },
	
	// Test normal one char and simple white space tokens
	{ "{ } ( ) [ ] , ; : =", 20, (token_t[]){
		{ .type = T_CBO,    .source = { 1, "{" } },
		{ .type = T_WS,     .source = { 1, " " } },
		{ .type = T_CBC,    .source = { 1, "}" } },
		{ .type = T_WS,     .source = { 1, " " } },
		{ .type = T_RBO,    .source = { 1, "(" } },
		{ .type = T_WS,     .source = { 1, " " } },
		{ .type = T_RBC,    .source = { 1, ")" } },
		{ .type = T_WS,     .source = { 1, " " } },
		{ .type = T_SBO,    .source = { 1, "[" } },
		{ .type = T_WS,     .source = { 1, " " } },
		{ .type = T_SBC,    .source = { 1, "]" } },
		{ .type = T_WS,     .source = { 1, " " } },
		{ .type = T_COMMA,  .source = { 1, "," } },
		{ .type = T_WS,     .source = { 1, " " } },
		{ .type = T_SEMI,   .source = { 1, ";" } },
		{ .type = T_WS,     .source = { 1, " " } },
		{ .type = T_COLON,  .source = { 1, ":" } },
		{ .type = T_WS,     .source = { 1, " " } },
		{ .type = T_ASSIGN, .source = { 1, "=" } },
		{ .type = T_EOF,    .source = { 0, "", } }
	} },
	
	// Test tab and new line (possible EOS) white space tokens
	{ " \t ( \n ) \t ", 6, (token_t[]){
		{ .type = T_WS,     .source = { 3, " \t " } },
		{ .type = T_RBO,    .source = { 1, "("    } },
		{ .type = T_WSNL,   .source = { 3, " \n " } },
		{ .type = T_RBC,    .source = { 1, ")"    } },
		{ .type = T_WS,     .source = { 3, " \t " } },
		{ .type = T_EOF,    .source = { 0, ""     } }
	} },
	
	// Integer literals
	{ "12345", 2, (token_t[]){
		{ .type = T_INT, .source = { 5, "12345" }, .int_val = 12345 },
		{ .type = T_EOF, .source = { 0, ""      }  }
	} },
	{ " 12345 ", 4, (token_t[]){
		{ .type = T_WS,  .source = { 1, " "     } },
		{ .type = T_INT, .source = { 5, "12345" }, .int_val = 12345 },
		{ .type = T_WS,  .source = { 1, " "     } },
		{ .type = T_EOF, .source = { 0, ""      } }
	} },
	
	// One line comments
	{ "// foo ", 2, (token_t[]){
		{ .type = T_COMMENT, .source = { 7, "// foo " } },
		{ .type = T_EOF,     .source = { 0, ""        } }
	} },
	{ " // foo \n ", 4, (token_t[]){
		{ .type = T_WS,      .source = { 1, " ",      } },
		{ .type = T_COMMENT, .source = { 7, "// foo " } },
		{ .type = T_WSNL,    .source = { 2, "\n ",    } },
		{ .type = T_EOF,     .source = { 0, "",       } }
	} },
	
	// Multiline comments
	{ "/* foo */", 2, (token_t[]){
		{ .type = T_COMMENT, .source = { 9, "/* foo */" } },
		{ .type = T_EOF,     .source = { 0, ""          } }
	} },
	{ "/**/", 2, (token_t[]){
		{ .type = T_COMMENT, .source = { 4, "/**/"      } },
		{ .type = T_EOF,     .source = { 0, ""          } }
	} },
	{ "/***/", 2, (token_t[]){
		{ .type = T_COMMENT, .source = { 5, "/***/"     } },
		{ .type = T_EOF,     .source = { 0, ""          } }
	} },
	{ "/****/", 2, (token_t[]){
		{ .type = T_COMMENT, .source = { 6, "/****/"    } },
		{ .type = T_EOF,     .source = { 0, ""          } }
	} },
	{ "/* s1 /* s2 /* foo */ e2 */ m1 /* s2 /* foo */ e2 */ m1 /*/*/*/*****/*/*/*/ e1  */", 2, (token_t[]){
		{ .type = T_COMMENT, .source = { 82, "/* s1 /* s2 /* foo */ e2 */ m1 /* s2 /* foo */ e2 */ m1 /*/*/*/*****/*/*/*/ e1  */" } },
		{ .type = T_EOF,     .source = { 0, ""          } }
	} },
	{ "/*", 2, (token_t[]){
		{ .type = T_ERROR,   .source = { 2, "/*"        } },
		{ .type = T_EOF,     .source = { 0, ""          } }
	} },
	{ " /**  /*", 3, (token_t[]){
		{ .type = T_WS,      .source = { 1, " "         } },
		{ .type = T_ERROR,   .source = { 7, "/**  /*"   } },
		{ .type = T_EOF,     .source = { 0, ""          } }
	} },
	{ " /* foo */ ", 4, (token_t[]){
		{ .type = T_WS,      .source = { 1, " "         } },
		{ .type = T_COMMENT, .source = { 9, "/* foo */" } },
		{ .type = T_WS,      .source = { 1, " "         } },
		{ .type = T_EOF,     .source = { 0, ""          } }
	} },
	{ " /***/ ", 4, (token_t[]){
		{ .type = T_WS,      .source = { 1, " "         } },
		{ .type = T_COMMENT, .source = { 5, "/***/"     } },
		{ .type = T_WS,      .source = { 1, " "         } },
		{ .type = T_EOF,     .source = { 0, ""          } }
	} },
	
	// Strings
	{ "\"foo\"", 2, (token_t[]){
		{ .type = T_STR, .source = { 5, "\"foo\""  }, .str_val = { 3, "foo" } },
		{ .type = T_EOF, .source = { 0, ""         } }
	} },
	{ "\"\\\\\"", 2, (token_t[]){
		{ .type = T_STR, .source = { 4, "\"\\\\\"" }, .str_val = { 1, "\\" } },
		{ .type = T_EOF, .source = { 0, ""         } }
	} },
	{ "\"\\t\"", 2, (token_t[]){
		{ .type = T_STR, .source = { 4, "\"\\t\""  }, .str_val = { 1, "\t" } },
		{ .type = T_EOF, .source = { 0, ""         } }
	} },
	{ "\"\\n\"", 2, (token_t[]){
		{ .type = T_STR, .source = { 4, "\"\\n\""  }, .str_val = { 1, "\n" } },
		{ .type = T_EOF, .source = { 0, ""         } }
	} },
	{ "\"\\\"\"", 2, (token_t[]){
		{ .type = T_STR, .source = { 4, "\"\\\"\"" }, .str_val = { 1, "\"" } },
		{ .type = T_EOF, .source = { 0, ""         } }
	} },
	{ "\"foo", 2, (token_t[]){
		{ .type = T_ERROR, .source = { 4, "\"foo"  } },
		{ .type = T_EOF,   .source = { 0, ""       } }
	} },
	{ "\"x\\", 2, (token_t[]){
		{ .type = T_ERROR, .source = { 3, "\"x\\"  } },
		{ .type = T_EOF,   .source = { 0, ""       } }
	} },
	{ "\"foo\\hbar\"", 3, (token_t[]){
		{ .type = T_ERROR, .source = {  2, "\\h"           } },
		{ .type = T_STR,   .source = { 10, "\"foo\\hbar\"" }, .str_val = { 6, "foobar" } },
		{ .type = T_EOF,   .source = {  0, ""              } }
	} },
	
	// IDs
	{ "foo", 2, (token_t[]){
		{ .type = T_ID,  .source = { 3, "foo" } },
		{ .type = T_EOF, .source = { 0, ""    } }
	} },
	{ "_12foo34", 2, (token_t[]){
		{ .type = T_ID,  .source = { 8, "_12foo34" } },
		{ .type = T_EOF, .source = { 0, ""         } }
	} },
	{ " foo ", 4, (token_t[]){
		{ .type = T_WS,  .source = { 1, " "   } },
		{ .type = T_ID,  .source = { 3, "foo" } },
		{ .type = T_WS,  .source = { 1, " "   } },
		{ .type = T_EOF, .source = { 0, ""    } }
	} },
	{ "foo bar", 4, (token_t[]){
		{ .type = T_ID,  .source = { 3, "foo" } },
		{ .type = T_WS,  .source = { 1, " "   } },
		{ .type = T_ID,  .source = { 3, "bar" } },
		{ .type = T_EOF, .source = { 0, ""    } }
	} },
	{ "+", 2, (token_t[]){
		{ .type = T_ADD, .source = { 1, "+"   } },
		{ .type = T_EOF, .source = { 0, ""    } }
	} },
	{ "foo+bar", 4, (token_t[]){
		{ .type = T_ID,  .source = { 3, "foo" } },
		{ .type = T_ADD, .source = { 1, "+"   } },
		{ .type = T_ID,  .source = { 3, "bar" } },
		{ .type = T_EOF, .source = { 0, ""    } }
	} },
	{ "-a+b*c/d", 9, (token_t[]){
		{ .type = T_SUB, .source = { 1, "-"   } },
		{ .type = T_ID,  .source = { 1, "a"   } },
		{ .type = T_ADD, .source = { 1, "+"   } },
		{ .type = T_ID,  .source = { 1, "b"   } },
		{ .type = T_MUL, .source = { 1, "*"   } },
		{ .type = T_ID,  .source = { 1, "c"   } },
		{ .type = T_DIV, .source = { 1, "/"   } },
		{ .type = T_ID,  .source = { 1, "d"   } },
		{ .type = T_EOF, .source = { 0, ""    } }
	} },
	
	// Operators
	{ " + += - -= * *= / /= % %= ", 22, (token_t[]){
		{ .type = T_WS,         .source = { 1, " "  } },
		{ .type = T_ADD,        .source = { 1, "+"  } },
		{ .type = T_WS,         .source = { 1, " "  } },
		{ .type = T_ADD_ASSIGN, .source = { 2, "+=" } },
		{ .type = T_WS,         .source = { 1, " "  } },
		{ .type = T_SUB,        .source = { 1, "-"  } },
		{ .type = T_WS,         .source = { 1, " "  } },
		{ .type = T_SUB_ASSIGN, .source = { 2, "-=" } },
		{ .type = T_WS,         .source = { 1, " "  } },
		{ .type = T_MUL,        .source = { 1, "*"  } },
		{ .type = T_WS,         .source = { 1, " "  } },
		{ .type = T_MUL_ASSIGN, .source = { 2, "*=" } },
		{ .type = T_WS,         .source = { 1, " "  } },
		{ .type = T_DIV,        .source = { 1, "/"  } },
		{ .type = T_WS,         .source = { 1, " "  } },
		{ .type = T_DIV_ASSIGN, .source = { 2, "/=" } },
		{ .type = T_WS,         .source = { 1, " "  } },
		{ .type = T_MOD,        .source = { 1, "%"  } },
		{ .type = T_WS,         .source = { 1, " "  } },
		{ .type = T_MOD_ASSIGN, .source = { 2, "%=" } },
		{ .type = T_WS,         .source = { 1, " "  } },
		{ .type = T_EOF,        .source = { 0, ""   } }
	} },
	{ " < <= << <<= > >= >> >>= ", 18, (token_t[]){
		{ .type = T_WS,        .source = { 1, " "   } },
		{ .type = T_LT,        .source = { 1, "<"   } },
		{ .type = T_WS,        .source = { 1, " "   } },
		{ .type = T_LE,        .source = { 2, "<="  } },
		{ .type = T_WS,        .source = { 1, " "   } },
		{ .type = T_SL,        .source = { 2, "<<"  } },
		{ .type = T_WS,        .source = { 1, " "   } },
		{ .type = T_SL_ASSIGN, .source = { 3, "<<=" } },
		{ .type = T_WS,        .source = { 1, " "   } },
		{ .type = T_GT,        .source = { 1, ">"   } },
		{ .type = T_WS,        .source = { 1, " "   } },
		{ .type = T_GE,        .source = { 2, ">="  } },
		{ .type = T_WS,        .source = { 1, " "   } },
		{ .type = T_SR,        .source = { 2, ">>"  } },
		{ .type = T_WS,        .source = { 1, " "   } },
		{ .type = T_SR_ASSIGN, .source = { 3, ">>=" } },
		{ .type = T_WS,        .source = { 1, " "   } },
		{ .type = T_EOF,       .source = { 0, ""    } }
	} },
	{ " & &= | |= ^ ^= ", 14, (token_t[]){
		{ .type = T_WS,             .source = { 1, " "  } },
		{ .type = T_BIT_AND,        .source = { 1, "&"  } },
		{ .type = T_WS,             .source = { 1, " "  } },
		{ .type = T_BIT_AND_ASSIGN, .source = { 2, "&=" } },
		{ .type = T_WS,             .source = { 1, " "  } },
		{ .type = T_BIT_OR,         .source = { 1, "|"  } },
		{ .type = T_WS,             .source = { 1, " "  } },
		{ .type = T_BIT_OR_ASSIGN,  .source = { 2, "|=" } },
		{ .type = T_WS,             .source = { 1, " "  } },
		{ .type = T_BIT_XOR,        .source = { 1, "^"  } },
		{ .type = T_WS,             .source = { 1, " "  } },
		{ .type = T_BIT_XOR_ASSIGN, .source = { 2, "^=" } },
		{ .type = T_WS,             .source = { 1, " "  } },
		{ .type = T_EOF,            .source = { 0, ""   } }
	} },
	{ " = == != . ~ ", 12, (token_t[]){
		{ .type = T_WS,     .source = { 1, " "  } },
		{ .type = T_ASSIGN, .source = { 1, "="  } },
		{ .type = T_WS,     .source = { 1, " "  } },
		{ .type = T_EQ,     .source = { 2, "==" } },
		{ .type = T_WS,     .source = { 1, " "  } },
		{ .type = T_NEQ,    .source = { 2, "!=" } },
		{ .type = T_WS,     .source = { 1, " "  } },
		{ .type = T_PERIOD, .source = { 1, "."  } },
		{ .type = T_WS,     .source = { 1, " "  } },
		{ .type = T_COMPL,  .source = { 1, "~"  } },
		{ .type = T_WS,     .source = { 1, " "  } },
		{ .type = T_EOF,    .source = { 0, ""   } }
	} },
	
	// Unknown char error
	{ " $ ", 4, (token_t[]){
		{ .type = T_WS,    .source = { 1, " " } },
		{ .type = T_ERROR, .source = { 1, "$" } },
		{ .type = T_WS,    .source = { 1, " " } },
		{ .type = T_EOF,   .source = { 0, ""  } }
	} },
};

void test_samples() {
	for(size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
		char* code = samples[i].code;
		//printf("test: %s\n", code);
		
		token_list_t tokens = { 0 };
		tokenize(str_from_c(code), &tokens, stderr);
		
		st_check_int(tokens.len, samples[i].tokens_len);
		for(size_t j = 0; j < samples[i].tokens_len; j++) {
			token_p actual_token = &tokens.ptr[j];
			token_p expected_token = &samples[i].tokens_ptr[j];
			
			st_check_msg(actual_token->type == expected_token->type, "got %s, expected %s",
				token_type_name(actual_token->type), token_type_name(expected_token->type));
			st_check_int(actual_token->source.len, expected_token->source.len);
			st_check_strn(actual_token->source.ptr, expected_token->source.ptr, expected_token->source.len);
			if (actual_token->type == T_INT) {
				st_check(actual_token->int_val == expected_token->int_val);
			} else if (actual_token->type == T_ERROR) {
				// Check that an error message is present, exact content doesn't matter, will change anyway
				st_check(actual_token->str_val.len > 0);
				st_check_not_null(actual_token->str_val.ptr);
			} else if (expected_token->str_val.ptr != NULL) {
				st_check_int(actual_token->str_val.len, expected_token->str_val.len);
				st_check_strn(actual_token->str_val.ptr, expected_token->str_val.ptr, expected_token->str_val.len);
			} else {
				st_check_null(actual_token->str_val.ptr);
				st_check_int(actual_token->str_val.len, 0);
			}
		}
		
		for(size_t j = 0; j < tokens.len; j++)
			token_cleanup(&tokens.ptr[j]);
		list_destroy(&tokens);
	}
}

// Don't test printing code to thoroughly because it will change a lot
void test_print_functions() {
	char*  output_ptr = NULL;
	size_t output_len = 0;
	FILE* output = NULL;
	
	node_p module = node_alloc(NT_MODULE);
	module->module.filename = str_from_c("tokenizer_test.c/test_print_functions");
	module->module.source = str_from_c("x = \n1 + y\n\"next\nline\"");
	
	tokenize(module->module.source, &module->tokens, stderr);
	st_check_int(module->tokens.len, 12);
	
	output = open_memstream(&output_ptr, &output_len);
		token_print(output, &module->tokens.ptr[10], TP_SOURCE);
	fclose(output);
	st_check_str(output_ptr, "\"next\nline\"");
	
	output = open_memstream(&output_ptr, &output_len);
		token_print(output, &module->tokens.ptr[10], TP_DUMP);
	fclose(output);
	st_check_not_null( strstr(output_ptr, "\"next\nline\"") );
	
	output = open_memstream(&output_ptr, &output_len);
		token_print(output, &module->tokens.ptr[10], TP_INLINE_DUMP);
	fclose(output);
	st_check_not_null( strstr(output_ptr, "\"next\\nline\"") );
}

void test_token_type_name() {
	st_check_str( token_type_name(T_COMMENT), "T_COMMENT" );
	st_check_str( token_type_name(T_SL_ASSIGN), "T_SL_ASSIGN" );
	st_check_str( token_type_name(T_EOF), "T_EOF" );
}

void test_token_desc() {
	st_check_str( token_desc(T_SL_ASSIGN), "<<="   );
	st_check_str( token_desc(T_COMMA),     "comma" );
	st_check_str( token_desc(T_FUNC),      "func"  );
	st_check_null( token_desc(T_COMMENT) );
	st_check_null( token_desc(T_ID) );
}


int main() {
	st_run(test_samples);
	st_run(test_print_functions);
	st_run(test_token_type_name);
	st_run(test_token_desc);
	return st_show_report();
}