#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"


int tok_dump(token_t token, char* buffer, size_t buffer_size) {
	switch(token.type) {
		case T_NONE:    snprintf(buffer, buffer_size, "NONE");                                                break;
		case T_WS:      snprintf(buffer, buffer_size, "ws(\"%.*s\")", (int)token.length, token.str_val);      break;
		case T_EOS:     snprintf(buffer, buffer_size, "eos(\"%.*s\")", (int)token.length, token.str_val);     break;
		case T_FUNC:    snprintf(buffer, buffer_size, "func");                                                break;
		case T_ID:      snprintf(buffer, buffer_size, "id(%.*s)", (int)token.length, token.str_val);          break;
		case T_CBO:     snprintf(buffer, buffer_size, "\"{\"");                                               break;
		case T_CBC:     snprintf(buffer, buffer_size, "\"}\"");                                               break;
		case T_RBO:     snprintf(buffer, buffer_size, "\"(\"");                                               break;
		case T_RBC:     snprintf(buffer, buffer_size, "\")\"");                                               break;
		case T_COMMA:   snprintf(buffer, buffer_size, "\",\"");                                               break;
		case T_ASSIGN:  snprintf(buffer, buffer_size, "\"=\"");                                               break;
		case T_RET:     snprintf(buffer, buffer_size, "return");                                              break;
		case T_INT:     snprintf(buffer, buffer_size, "int(%d)", token.int_val);                              break;
		case T_EOF:     snprintf(buffer, buffer_size, "EOF");                                                 break;
		case T_COMMENT: snprintf(buffer, buffer_size, "comment(\"%.*s\")", (int)token.length, token.str_val); break;
		case T_STR:     snprintf(buffer, buffer_size, "str(\"%.*s\")", (int)token.length, token.str_val);     break;
	}
	
	return -1;
}

struct { const char* keyword; token_type_t type; } keywords[] = {
	{ "function", T_FUNC },
	{ "return", T_RET }
};

uint32_t line = 1, col = 1, prev_col = 0;

void tok_init() {
	line = 1;
	col = 1;
	prev_col = 0;
}

token_t tok_next(FILE* input, FILE* errors) {
	token_t init_token(token_type_t type, size_t length) {
		return (token_t){
			.type = type,
			.line = line,
			.col = col,
			.offset = ftell(input),
			.length = length
		  };
	}
	
	void tok_str_append(token_p token, char c) {
		token->length++;
		token->str_val = realloc(token->str_val, token->length);
		token->str_val[token->length-1] = c;
	}
	
	int consume_char() {
		int c = getc(input);
		if (c == '\n') {
			line++;
			prev_col = col;
			col = 1;
		} else {
			col++;
		}
		return c;
	}
	
	void unconsume_char(int c) {
		if (c == '\n') {
			line--;
			col = prev_col;
		} else {
			col--;
		}
		ungetc(c, input);
	}
	
	int c = consume_char(input);
	switch(c) {
		case EOF:
			return init_token(T_EOF, 0);
		case '{':
			return init_token(T_CBO, 1);
		case '}':
			return init_token(T_CBC, 1);
		case '(':
			return init_token(T_RBO, 1);
		case ')':
			return init_token(T_RBO, 1);
		case ',':
			return init_token(T_COMMA, 1);
		case '=':
			return init_token(T_ASSIGN, 1);
	}
	
	if ( isspace(c) ) {
		token_t t = init_token(T_WS, 0);
		
		do {
			tok_str_append(&t, c);
			// If a white space token contains a new line it becomes a possible end of statement
			if (c == '\n')
				t.type = T_EOS;
			c = consume_char(input);
		} while ( isspace(c) );
		unconsume_char(c);
		
		return t;
	}
	
	if ( c != '0' && isdigit(c) ) {
		token_t t = init_token(T_INT, 0);
		
		int value = 0;
		do {
			value = value * 10 + (c - '0');
			c = consume_char(input);
		} while ( isdigit(c) );
		unconsume_char(c);
		
		t.int_val = value;
		return t;
	}
	
	if ( c == '/' ) {
		int prev_c = c;
		c = consume_char(input);
		
		if (c == '/') {
			token_t t = init_token(T_COMMENT, 0);
			tok_str_append(&t, '/');
			
			do {
				tok_str_append(&t, c);
				c = consume_char(input);
			} while ( !(c == '\n' || c == EOF) );
			unconsume_char(c);
			
			return t;
		} else if (c == '*') {
			token_t t = init_token(T_COMMENT, 0);
			tok_str_append(&t, '/');
			tok_str_append(&t, '*');
			
			int nesting_level = 1;
			while ( nesting_level > 0 ) {
				c = consume_char(input);
				tok_str_append(&t, c);
				
				if (c == '*') {
					c = consume_char(input);
					tok_str_append(&t, c);
					
					if (c == '/') {
						nesting_level--;
					}
				} else if (c == '/') {
					c = consume_char(input);
					tok_str_append(&t, c);
					
					if (c == '*') {
						nesting_level++;
					}
				}
			}
			
			return t;
		}
		
		// Not a comment, put the peeked char back into stream so the / will be matched as an T_ID
		unconsume_char(c);
		c = prev_c;
	}
	
	if (c == '"') {
		token_t t = init_token(T_STR, 0);
		
		while(1) {
			c = consume_char(input);
			switch(c) {
				case EOF:
					fprintf(errors, "unterminated string!\n");
					return t;
				case '"':
					return t;
				case '\\':
					c = consume_char(input);
					switch(c) {
						case EOF:
							fprintf(errors, "unterminated escape code in string!\n");
							return t;
						case '\\':
							tok_str_append(&t, '\\');
							break;
						case '"':
							tok_str_append(&t, '"');
							break;
						case 'n':
							tok_str_append(&t, '\n');
							break;
						case 't':
							tok_str_append(&t, '\t');
							break;
						default:
							fprintf(errors, "unknown escape code in string: \\%c, ignoring\n", c);
							break;
					}
					break;
				default:
					tok_str_append(&t, c);
					break;
			}
		}
	}
	
	if ( isalpha(c) || c == '+' || c == '-' || c == '*' || c == '/' ) {
		token_t t = init_token(T_ID, 0);
		
		do {
			tok_str_append(&t, c);
			c = consume_char(input);
		} while ( isalnum(c) );
		unconsume_char(c);
		
		for(size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
			if ( strncmp(t.str_val, keywords[i].keyword, t.length) == 0 ) {
				t.type = keywords[i].type;
				break;
			}
		}
		
		return t;
	}
	
	fprintf(errors, "Unknown character: '%c' (%d), ignoring\n", c, c);
	return tok_next(input, errors);
}

void tok_free(token_t token) {
	switch(token.type) {
		case T_WS:
		case T_EOS:
		case T_FUNC:
		case T_ID:
		case T_RET:
		case T_COMMENT:
		case T_STR:
			free(token.str_val);
			break;
		default:
			break;
	}
}