#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>




typedef enum {
	T_COMMENT,
	T_WS,
	T_WS_EOS,
	
	T_STR,
	T_INT,
	T_ID,
	
	T_CBO,    // {
	T_CBC,    // }
	T_RBO,    // (
	T_RBC,    // )
	T_COMMA,  // ,
	T_ASSIGN, // =
	
	T_FUNC,
	T_RET,
	
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



/*
typedef struct tokenized_file_s tokenized_file_t, *tokenized_file_p;

typedef struct {
	token_type_t type;
	int line;
	char* source;
	int size;
	tokenized_file_p file;
	union {
		struct { char* str_val; int str_size; };
		int int_val;
	};
} token_t, *token_p;

struct tokenized_file_s {
	const char* name;
	char* source;
	size_t source_size;
	token_p tokens;
	size_t token_count;
};


tokenized_file_p tokenize_str(char* source, size_t size, const char* name, FILE* errors);
void             tokenized_file_free(tokenized_file_p file);

int  token_dump(token_p token, char* buffer, size_t buffer_size);
void token_print_line(token_p token);
*/