#pragma once

#include <stddef.h>


typedef struct {
	int fd;
	const char* data;
	size_t length;
	
	size_t pos;
	//size_t line, col;
} scanner_t, *scanner_p;

typedef enum {
	T_BEGIN, T_END, T_READ, T_WRITE, T_ID, T_INTLITERAL,
	T_LPAREN, T_RPAREN, T_SEMICOLON, T_COMMA, T_ASSIGNOP,
	T_PLUSOP, T_MINUSOP, T_EOF, T_ERROR
} token_type_t;

typedef struct {
	token_type_t type;
	const char* text;
	size_t length;
	//size_t line, col;
} token_t, *token_p;


scanner_t scanner_new(const char* filename);
scanner_t scanner_new_from_string(const char* code);
void      scanner_destroy(scanner_p scan);
token_t   scanner_read(scanner_p scan);