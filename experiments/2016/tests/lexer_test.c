#include <string.h>
#include "../lexer.h"

#define SLIM_TEST_IMPLEMENTATION
#include "slim_test.h"


struct { char* code; size_t tokens_len; token_p tokens_ptr; } samples[] = {
	// Empty string
	{ "", 1, (token_t[]){
		{ .type = T_EOF, .src_str = "", .src_len = 0 }
	} },
	
	// Test normal one char and simple white space tokens
	{ "{ } ( ) , =", 12, (token_t[]){
		{ .type = T_CBO, .src_str = "{", .src_len = 1 },
		{ .type = T_WS,  .src_str = " ", .src_len = 1 },
		{ .type = T_CBC, .src_str = "}", .src_len = 1 },
		{ .type = T_WS,  .src_str = " ", .src_len = 1 },
		{ .type = T_RBO, .src_str = "(", .src_len = 1 },
		{ .type = T_WS,  .src_str = " ", .src_len = 1 },
		{ .type = T_RBC, .src_str = ")", .src_len = 1 },
		{ .type = T_WS,  .src_str = " ", .src_len = 1 },
		{ .type = T_COMMA,  .src_str = ",", .src_len = 1 },
		{ .type = T_WS,     .src_str = " ", .src_len = 1 },
		{ .type = T_ASSIGN, .src_str = "=", .src_len = 1 },
		{ .type = T_EOF,    .src_str = "",  .src_len = 0 }
	} },
	
	// Test tab and new line (possible EOS) white space tokens
	{ " \t ( \n ) \t ", 6, (token_t[]){
		{ .type = T_WS,      .src_str = " \t ", .src_len = 3 },
		{ .type = T_RBO,     .src_str = "(",    .src_len = 1 },
		{ .type = T_WS_EOS,  .src_str = " \n ", .src_len = 3 },
		{ .type = T_RBC,     .src_str = ")",    .src_len = 1 },
		{ .type = T_WS,      .src_str = " \t ", .src_len = 3 },
		{ .type = T_EOF,     .src_str = "",     .src_len = 0 }
	} },
	
	// Integer literals
	{ "12345", 2, (token_t[]){
		{ .type = T_INT, .src_str = "12345", .src_len = 5, .int_val = 12345 },
		{ .type = T_EOF, .src_str = "",      .src_len = 0 }
	} },
	{ " 12345 ", 4, (token_t[]){
		{ .type = T_WS,  .src_str = " ", .src_len = 1 },
		{ .type = T_INT, .src_str = "12345", .src_len = 5, .int_val = 12345 },
		{ .type = T_WS,  .src_str = " ", .src_len = 1 },
		{ .type = T_EOF, .src_str = "",  .src_len = 0 }
	} },
	
	// One line comments
	{ "// foo ", 2, (token_t[]){
		{ .type = T_COMMENT, .src_str = "// foo ", .src_len = 7 },
		{ .type = T_EOF,     .src_str = "",        .src_len = 0 }
	} },
	{ " // foo \n ", 4, (token_t[]){
		{ .type = T_WS,      .src_str = " ",       .src_len = 1 },
		{ .type = T_COMMENT, .src_str = "// foo ", .src_len = 7 },
		{ .type = T_WS_EOS,  .src_str = "\n ",     .src_len = 2 },
		{ .type = T_EOF,     .src_str = "",        .src_len = 0 }
	} },
	
	// Multiline comments
	{ "/* foo */", 2, (token_t[]){
		{ .type = T_COMMENT, .src_str = "/* foo */", .src_len = 9 },
		{ .type = T_EOF,     .src_str = "",          .src_len = 0 }
	} },
	{ "/**/", 2, (token_t[]){
		{ .type = T_COMMENT, .src_str = "/**/",      .src_len = 4 },
		{ .type = T_EOF,     .src_str = "",          .src_len = 0 }
	} },
	{ "/***/", 2, (token_t[]){
		{ .type = T_COMMENT, .src_str = "/***/",     .src_len = 5 },
		{ .type = T_EOF,     .src_str = "",          .src_len = 0 }
	} },
	{ "/****/", 2, (token_t[]){
		{ .type = T_COMMENT, .src_str = "/****/",    .src_len = 6 },
		{ .type = T_EOF,     .src_str = "",          .src_len = 0 }
	} },
	{ "/* s1 /* s2 /* foo */ e2 */ m1 /* s2 /* foo */ e2 */ m1 /*/*/*/*****/*/*/*/ e1  */", 2, (token_t[]){
		{ .type = T_COMMENT, .src_str = "/* s1 /* s2 /* foo */ e2 */ m1 /* s2 /* foo */ e2 */ m1 /*/*/*/*****/*/*/*/ e1  */", .src_len = 82 },
		{ .type = T_EOF,     .src_str = "",          .src_len = 0 }
	} },
	{ "/*", 2, (token_t[]){
		{ .type = T_ERROR,   .src_str = "/*",        .src_len = 2, .str_len = 30, .str_val = "unterminated multiline comment" },
		{ .type = T_EOF,     .src_str = "",          .src_len = 0 }
	} },
	{ " /**  /*", 3, (token_t[]){
		{ .type = T_WS,      .src_str = " ",         .src_len = 1 },
		{ .type = T_ERROR,   .src_str = "/**  /*",   .src_len = 7, .str_len = 30, .str_val = "unterminated multiline comment" },
		{ .type = T_EOF,     .src_str = "",          .src_len = 0 }
	} },
	{ " /* foo */ ", 4, (token_t[]){
		{ .type = T_WS,      .src_str = " ",         .src_len = 1 },
		{ .type = T_COMMENT, .src_str = "/* foo */", .src_len = 9 },
		{ .type = T_WS,      .src_str = " ",         .src_len = 1 },
		{ .type = T_EOF,     .src_str = "",          .src_len = 0 }
	} },
	{ " /***/ ", 4, (token_t[]){
		{ .type = T_WS,      .src_str = " ",         .src_len = 1 },
		{ .type = T_COMMENT, .src_str = "/***/",     .src_len = 5 },
		{ .type = T_WS,      .src_str = " ",         .src_len = 1 },
		{ .type = T_EOF,     .src_str = "",          .src_len = 0 }
	} },
	
	// Strings
	{ "\"foo\"", 2, (token_t[]){
		{ .type = T_STR, .src_str = "\"foo\"", .src_len = 5, .str_len = 3, .str_val = "foo" },
		{ .type = T_EOF, .src_str = "",        .src_len = 0 }
	} },
	{ "\"\\\\\"", 2, (token_t[]){
		{ .type = T_STR, .src_str = "\"\\\\\"", .src_len = 4, .str_len = 1, .str_val = "\\" },
		{ .type = T_EOF, .src_str = "",         .src_len = 0 }
	} },
	{ "\"\\t\"", 2, (token_t[]){
		{ .type = T_STR, .src_str = "\"\\t\"", .src_len = 4, .str_len = 1, .str_val = "\t" },
		{ .type = T_EOF, .src_str = "",        .src_len = 0 }
	} },
	{ "\"\\n\"", 2, (token_t[]){
		{ .type = T_STR, .src_str = "\"\\n\"", .src_len = 4, .str_len = 1, .str_val = "\n" },
		{ .type = T_EOF, .src_str = "",        .src_len = 0 }
	} },
	{ "\"\\\"\"", 2, (token_t[]){
		{ .type = T_STR, .src_str = "\"\\\"\"", .src_len = 4, .str_len = 1, .str_val = "\"" },
		{ .type = T_EOF, .src_str = "",         .src_len = 0 }
	} },
	{ "\"foo", 2, (token_t[]){
		{ .type = T_ERROR, .src_str = "\"foo",  .src_len = 4, .str_len = 19, .str_val = "unterminated string" },
		{ .type = T_EOF,   .src_str = "",       .src_len = 0 }
	} },
	{ "\"x\\", 2, (token_t[]){
		{ .type = T_ERROR, .src_str = "\"x\\",  .src_len = 3, .str_len = 34, .str_val = "unterminated escape code in string" },
		{ .type = T_EOF,   .src_str = "",       .src_len = 0 }
	} },
	/*
	{ "\"foo\\hbar\"", 2, (token_t[]){
		{ .type = T_ERROR, .src_str = "\"foo\\hbar\"",  .src_len = 9, .str_len = 29, .str_val = "unknown escape code in string" },
		{ .type = T_EOF,   .src_str = "",              .src_len = 0 }
	} },
	*/
	
	// IDs
	{ "foo", 2, (token_t[]){
		{ .type = T_ID,  .src_str = "foo", .src_len = 3 },
		{ .type = T_EOF, .src_str = "",    .src_len = 0 }
	} },
	{ "_12foo34", 2, (token_t[]){
		{ .type = T_ID,  .src_str = "_12foo34", .src_len = 8 },
		{ .type = T_EOF, .src_str = "",         .src_len = 0 }
	} },
	{ " foo ", 4, (token_t[]){
		{ .type = T_WS,  .src_str = " ",   .src_len = 1 },
		{ .type = T_ID,  .src_str = "foo", .src_len = 3 },
		{ .type = T_WS,  .src_str = " ",   .src_len = 1 },
		{ .type = T_EOF, .src_str = "",    .src_len = 0 }
	} },
	{ "foo bar", 4, (token_t[]){
		{ .type = T_ID,  .src_str = "foo", .src_len = 3 },
		{ .type = T_WS,  .src_str = " ",   .src_len = 1 },
		{ .type = T_ID,  .src_str = "bar", .src_len = 3 },
		{ .type = T_EOF, .src_str = "",    .src_len = 0 }
	} },
	{ "+", 2, (token_t[]){
		{ .type = T_ID,  .src_str = "+",   .src_len = 1 },
		{ .type = T_EOF, .src_str = "",    .src_len = 0 }
	} },
	{ "foo+bar", 4, (token_t[]){
		{ .type = T_ID,  .src_str = "foo", .src_len = 3 },
		{ .type = T_ID,  .src_str = "+",   .src_len = 1 },
		{ .type = T_ID,  .src_str = "bar", .src_len = 3 },
		{ .type = T_EOF, .src_str = "",    .src_len = 0 }
	} },
	{ "-a+b*c/d", 9, (token_t[]){
		{ .type = T_ID,  .src_str = "-",   .src_len = 1 },
		{ .type = T_ID,  .src_str = "a",   .src_len = 1 },
		{ .type = T_ID,  .src_str = "+",   .src_len = 1 },
		{ .type = T_ID,  .src_str = "b",   .src_len = 1 },
		{ .type = T_ID,  .src_str = "*",   .src_len = 1 },
		{ .type = T_ID,  .src_str = "c",   .src_len = 1 },
		{ .type = T_ID,  .src_str = "/",   .src_len = 1 },
		{ .type = T_ID,  .src_str = "d",   .src_len = 1 },
		{ .type = T_EOF, .src_str = "",    .src_len = 0 }
	} },
	
	// Unknown char error
	{ " $ ", 4, (token_t[]){
		{ .type = T_WS,    .src_str = " ", .src_len = 1 },
		{ .type = T_ERROR, .src_str = "$", .src_len = 1, .str_len = 30, .str_val = "stray character in source code" },
		{ .type = T_WS,    .src_str = " ", .src_len = 1 },
		{ .type = T_EOF,   .src_str = "",  .src_len = 0 }
	} },
};

void test_samples() {
	for(size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); i++) {
		char* code = samples[i].code;
		printf("test: %s\n", code);
		token_list_p tokens = lex_str(code, strlen(code), "sample_code", stderr);
		
		st_check_int(tokens->tokens_len, samples[i].tokens_len);
		for(size_t j = 0; j < samples[i].tokens_len; j++) {
			token_p actual_token = &tokens->tokens_ptr[j];
			token_p expected_token = &samples[i].tokens_ptr[j];
			
			st_check(actual_token->list == tokens);
			st_check_int(actual_token->type, expected_token->type);
			st_check_int(actual_token->src_len, expected_token->src_len);
			st_check_strn(actual_token->src_str, expected_token->src_str, expected_token->src_len);
			if (actual_token->type == T_INT) {
				st_check(actual_token->int_val == expected_token->int_val);
			} else if (expected_token->str_val != NULL) {
				st_check_int(actual_token->str_len, expected_token->str_len);
				st_check_strn(actual_token->str_val, expected_token->str_val, expected_token->str_len);
			} else {
				st_check_null(actual_token->str_val);
				st_check_int(actual_token->str_len, 0);
			}
		}
		
		lex_free(tokens);
	}
}


int main() {
	st_run(test_samples);
	return st_show_report();
}