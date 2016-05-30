#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "utils.h"


typedef enum {
	T_COMMENT,  // /* //
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
	
	// Tokens for unary and binary operators
	T_ADD, T_ADD_ASSIGN,  // + +=
	T_SUB, T_SUB_ASSIGN,  // - -=
	T_MUL, T_MUL_ASSIGN,  // * *=
	T_DIV, T_DIV_ASSIGN,  // / /=   // and /* become T_COMMENT
	T_MOD, T_MOD_ASSIGN,  // % %=
	
	T_LT, T_LE, T_SL, T_SL_ASSIGN,   // < <= << <<=
	T_GT, T_GE, T_SR, T_SR_ASSIGN,   // > >= >> >>=
	
	T_BIN_AND, T_BIN_AND_ASSIGN,  // & &=    && becomes T_AND
	T_BIN_OR,  T_BIN_OR_ASSIGN,   // | |=    || becomes T_OR
	T_BIN_XOR, T_BIN_XOR_ASSIGN,  // ^ ^=
	
	T_ASSIGN, T_EQ,     // = ==
	T_NEQ,              // !=    ! becomes T_NOT
	T_PERIOD, T_COMPL,  // . ~
	
	// Keywords
	T_NOT, T_AND, T_OR,
	T_SYSCALL,
	T_VAR,
	T_IF, T_THEN, T_ELSE,
	T_WHILE, T_DO,
	
	T_ERROR,
	T_EOF
} token_type_t;

typedef struct token_s token_t, *token_p;
typedef struct token_list_s token_list_t, *token_list_p;

struct token_s {
	token_list_p list;
	
	token_type_t type;
	str_t src;
	
	union {
		str_t   str_val;
		int64_t int_val;
	};
};

struct token_list_s {
	str_t src;
	const char* filename;
	
	list_t(token_t) tokens;
	size_t  error_count;
};


token_list_p lex_str(str_t code, const char* filename, FILE* errors);
void         lex_free(token_list_p token_list);

#define TP_SOURCE      (1 << 0)  // print only source
#define TP_DUMP        (1 << 1)  // print type and source
#define TP_INLINE_DUMP (1 << 2)  // print type and escaped and shorted source to avoid line breaks in the output

void token_print(FILE* stream, token_p token, uint32_t flags);
void token_print_line(FILE* stream, token_p token, int first_line_indent);
int  token_line(token_p token);
int  token_col(token_p token);