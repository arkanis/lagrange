#include <stdbool.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "scanner.h"


static bool    at_eof(scanner_p scan);
static int     get(scanner_p scan);
static void    unget(scanner_p scan);
static token_t id_or_reserved(token_t token);
static token_t error(scanner_p scan);


scanner_t scanner_new(const char* filename){
	scanner_t scan;
	
	scan.fd = open(filename, O_RDONLY, 0);
	
	struct stat file_stat;
	fstat(scan.fd, &file_stat);
	scan.length = file_stat.st_size;
	scan.data = mmap(NULL, scan.length, PROT_READ, MAP_PRIVATE, scan.fd, 0);
	
	scan.pos = 0;
	//scan.line = 1;
	//scan.col = 1;
	
	return scan;
}

scanner_t scanner_new_from_string(const char* code){
	scanner_t scan;
	
	scan.fd = -1;
	scan.data = code;
	scan.length = strlen(code);
	
	scan.pos = 0;
	//scan.line = 1;
	//scan.col = 1;
	
	return scan;
}

void scanner_destroy(scanner_p scan){
	if (scan->fd >= 0){
		munmap((void*)scan->data, scan->length);
		close(scan->fd);
	}
}

token_t scanner_read(scanner_p scan){
	token_t token_at_current_pos(token_type_t type){
		return (token_t){ type, scan->data + scan->pos, 1 };
	}
	
	int in;
	token_t tok;
	
	if ( at_eof(scan) )
		return (token_t){.type = T_EOF, .text = NULL, .length = 0 };
	
	while ( (in = get(scan)) != EOF ) {
		if ( isspace(in) ) {
			// ignore whitespaces for now
			continue;
		} else if ( isalpha(in) ) {
			// identifier
			tok = token_at_current_pos(T_INTLITERAL);
			for(int c = get(scan); isalnum(c) || c == '_'; c = get(scan))
				tok.length++;
			unget(scan);
			return id_or_reserved(tok);
		} else if ( isdigit(in) ) {
			// int literal
			tok = token_at_current_pos(T_INTLITERAL);
			for(int c = get(scan); isdigit(c) || c == '_'; c = get(scan))
				tok.length++;
			unget(scan);
			return tok;
		} else if (in == ':') {
			// might be :=
			tok = token_at_current_pos(T_ASSIGNOP);
			
			int c = get(scan);
			if (c == '=') {
				tok.length++;
				return tok;
			} else {
				unget(scan);
				error(scan);
			}
		} else if (in == '-') {
			// comment (--) or minus (-)
			tok = token_at_current_pos(T_MINUSOP);
			
			int c = get(scan);
			if (c == '-') {
				do {
					tok.length++;
					in = get(scan);
				} while (in != '\n');
			} else {
				unget(scan);
				return tok;
			}
		} else {
			switch(in){
				case '(':
					return token_at_current_pos(T_LPAREN);
				case ')':
					return token_at_current_pos(T_RPAREN);
				case ';':
					return token_at_current_pos(T_SEMICOLON);
				case ',':
					return token_at_current_pos(T_COMMA);
				case '+':
					return token_at_current_pos(T_PLUSOP);
				default:
					return error(scan);
			}
			break;
		}
	}
	
	return error(scan);
}


static bool at_eof(scanner_p scan){
	return (scan->pos >= scan->length);
}

static int get(scanner_p scan){
	if ( at_eof(scan) )
		return EOF;
	return scan->data[scan->pos++];
}

static void unget(scanner_p scan){
	scan->pos--;
}

static token_t id_or_reserved(token_t token){
	struct{ const char* name; token_type_t type; } keywords[] = {
		{ "begin", T_BEGIN },
		{ "end",   T_END   },
		{ "read",  T_READ  },
		{ "write", T_WRITE },
		{ NULL,    -1      }
	};
	
	for(size_t i = 0; keywords[i].name != NULL; i++){
		if ( strncmp(keywords[i].name, token.text, token.length) == 0 ){
			token.type = keywords[i].type;
			break;
		}
	}
	
	return token;
}

static token_t error(scanner_p scan){
	printf("error at pos %zu\n", scan->pos);
	return (token_t){ T_ERROR, scan->data + scan->pos, 1 };
}