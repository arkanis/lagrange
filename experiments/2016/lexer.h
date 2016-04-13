#pragma once
#include <stdint.h>

typedef enum {
	T_NONE,
	T_COMMENT,
	T_WS,
	
	T_EOS,
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


typedef struct {
	token_type_t type;
	uint32_t line, col;
	size_t offset, length;
	union {
		char* str_val;
		int int_val;
	};
} token_t, *token_p;

void tok_init();
token_t tok_next(FILE* input, FILE* errors);
void tok_free(token_t token);

int tok_dump(token_t token, char* buffer, size_t buffer_size);