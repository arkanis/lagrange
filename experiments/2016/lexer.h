#pragma once
#include <stdint.h>


typedef enum {
	T_COMMENT,
	T_WS,
	T_WS_EOS,
	
	T_FUNC,
	T_ID,
	T_CBO, // {
	T_CBC, // }
	T_RBO, // (
	T_RBC, // )
	T_COMMA, // ,
	T_ASSIGN, // =
	T_RET,
	T_INT,
	T_STR,
	
	T_EOF
} token_type_t;


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