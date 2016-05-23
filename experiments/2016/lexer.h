#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>




typedef enum {
	T_COMMENT,
	T_WS,
	T_WSNL,
	
	T_STR,
	T_INT,
	T_ID,
	
	T_CBO,    // {
	T_CBC,    // }
	T_RBO,    // (
	T_RBC,    // )
	T_COMMA,  // ,
	T_ASSIGN, // =
	
	T_SYSCALL,
	T_VAR,
	
	T_ERROR,
	T_EOF
} token_type_t;

typedef struct token_s token_t, *token_p;
typedef struct token_list_s token_list_t, *token_list_p;

struct token_s {
	token_list_p list;
	
	token_type_t type;
	char* src_str;
	int   src_len;
	
	union {
		struct {
			char* str_val;
			int   str_len;
		};
		int64_t int_val;
	};
};

struct token_list_s {
	char* src_str;
	int   src_len;
	const char* filename;
	
	token_p tokens_ptr;
	size_t  tokens_len;
	size_t  error_count;
};


token_list_p lex_str(char* code_str, int code_len, const char* filename, FILE* errors);
void         lex_free(token_list_p token_list);

#define TP_SOURCE      (1 << 0)  // print only source
#define TP_DUMP        (1 << 1)  // print type and source
#define TP_INLINE_DUMP (1 << 2)  // print type and escaped and shorted source to avoid line breaks in the output

void token_print(FILE* stream, token_p token, uint32_t flags);
void token_print_line(FILE* stream, token_p token, int first_line_indent);
int  token_line(token_p token);
int  token_col(token_p token);