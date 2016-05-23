#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "lexer.h"


//
// Token printing functions
//

static void token_print_escaped_str(FILE* stream, const char* str, int len) {
	for(int i = 0; i < len && i < 20 && str[i] != '\0'; i++) {
		switch(str[i]) {
			case '\n': fprintf(stream, "\\n"); break;
			case '\t': fprintf(stream, "\\t"); break;
			default:   putc(str[i], stream);   break;
		}
	}
}

void token_print(FILE* stream, token_p token, uint32_t flags) {
	if (flags & TP_SOURCE) {
		fprintf(stream, "%.*s", token->src_len, token->src_str);
	} else if ( (flags & TP_DUMP) || (flags & TP_INLINE_DUMP) ) {
		switch(token->type) {
			case T_COMMENT:
				fprintf(stream, "comment(");
				if (flags & TP_DUMP)
					fprintf(stream, "%.*s", token->src_len, token->src_str);
				else
					token_print_escaped_str(stream, token->src_str, token->str_len);
				fprintf(stream, ")");
				break;
			case T_WS:
				fprintf(stream, "ws(\"");
				if (flags & TP_DUMP)
					fprintf(stream, "%.*s", token->src_len, token->src_str);
				else
					token_print_escaped_str(stream, token->src_str, token->str_len);
				fprintf(stream, "\")");
				break;
			case T_WSNL:
				fprintf(stream, "ws_eos(\"");
				if (flags & TP_DUMP)
					fprintf(stream, "%.*s", token->src_len, token->src_str);
				else
					token_print_escaped_str(stream, token->src_str, token->str_len);
				fprintf(stream, "\")");
				break;
			
			case T_STR:
				fprintf(stream, "str(\"");
				if (flags & TP_DUMP)
					fprintf(stream, "%.*s", token->str_len, token->str_val);
				else
					token_print_escaped_str(stream, token->str_val, token->str_len);
				fprintf(stream, "\")");
				break;
			case T_INT:     fprintf(stream, "int(%ld)",       token->int_val);                  break;
			case T_ID:      fprintf(stream, "id(%.*s)",      token->src_len, token->src_str);  break;
			
			case T_CBO:     fprintf(stream, "\"{\"");  break;
			case T_CBC:     fprintf(stream, "\"}\"");  break;
			case T_RBO:     fprintf(stream, "\"(\"");  break;
			case T_RBC:     fprintf(stream, "\")\"");  break;
			case T_COMMA:   fprintf(stream, "\",\"");  break;
			case T_ASSIGN:  fprintf(stream, "\"=\"");  break;
			
			case T_SYSCALL: fprintf(stream, "syscall");  break;
			case T_VAR:     fprintf(stream, "var");      break;
			
			case T_ERROR:   fprintf(stream, "error(%.*s)", token->str_len, token->str_val);  break;
			case T_EOF:     fprintf(stream, "EOF");                                          break;
		}
	}
}

void token_print_line(FILE* stream, token_p token, int first_line_indent) {
	char* line_start = token->src_str;
	while (line_start > token->list->src_str && *(line_start-1) != '\n')
		line_start--;
	char* line_end = token->src_str + token->src_len;
	while (*line_end != '\n' && line_end < token->list->src_str + token->list->src_len)
		line_end++;
	
	fprintf(stream, "%.*s\e[1;4m%.*s\e[0m%.*s\n",
		(int)(token->src_str - line_start), line_start,
		token->src_len, token->src_str,
		(int)(line_end - (token->src_str + token->src_len)), token->src_str + token->src_len
	);
}

int token_line(token_p token) {
	int line = 1;
	for(char* c = token->src_str; c >= token->list->src_str; c--) {
		if (*c == '\n')
			line++;
	}
	return line;
}

int token_col(token_p token) {
	int col = 0;
	for(char* c = token->src_str; *c != '\n' && c >= token->list->src_str; c--)
		col++;
	return col;
}


//
// Private lexer support functions
//

typedef struct {
	size_t pos;
	token_list_p tokens;
	FILE* errors;
} lexer_t, *lexer_p;

static int peek_at_offset(lexer_p lexer, size_t offset) {
	if (lexer->pos + offset >= (size_t)lexer->tokens->src_len)
		return EOF;
	return lexer->tokens->src_str[lexer->pos + offset];
}

static int peek1(lexer_p lexer) {
	return peek_at_offset(lexer, 0);
}

static int peek2(lexer_p lexer) {
	return peek_at_offset(lexer, 1);
}

static token_t new_token(lexer_p lexer, token_type_t type, int chars_to_consume) {
	if (lexer->pos + chars_to_consume > (size_t)lexer->tokens->src_len) {
		fprintf(lexer->errors, "Tried to consume a char beyond EOF!\n");
		abort();
	}
	
	token_t token = (token_t){
		.list = lexer->tokens,
		
		.type = type,
		.src_str = lexer->tokens->src_str + lexer->pos,
		.src_len = chars_to_consume,
		
		.str_val = NULL,
		.str_len = 0
	};
	lexer->pos += chars_to_consume;
	return token;
}

static void putc_into_token(lexer_p lexer, token_p token, int c) {
	// Ignore any EOF that are put into a token
	// TODO: Go idea? Might better abort() to catch errors and enless loops more easier.
	if (c == EOF)
		return;
	
	if (lexer->pos + 1 > (size_t)lexer->tokens->src_len) {
		fprintf(lexer->errors, "Tried to consume a char beyond EOF!\n");
		abort();
	} else if (lexer->tokens->src_str + lexer->pos != token->src_str + token->src_len) {
		fprintf(lexer->errors, "Tried to put a char into a token thats end isn't the current lexer position!\n");
		abort();
	} else if (lexer->tokens->src_str[lexer->pos] != c) {
		fprintf(lexer->errors, "Tried to consume a char ('%c') that's not the current char ('%c')\n", c, lexer->tokens->src_str[lexer->pos]);
		abort();
	}
	
	lexer->pos++;
	token->src_len++;
}

static void append_token_str(token_p token, char c) {
	token->str_len++;
	token->str_val = realloc(token->str_val, token->str_len);
	token->str_val[token->str_len-1] = c;
}

static void make_into_error_token(token_p token, char* message) {
	token->type = T_ERROR;
	token->str_val = message;
	token->str_len = strlen(message);
}


//
// Public lexer functions
//

static token_t next_token(lexer_p lexer);

token_list_p lex_str(char* src_str, int src_len, const char* filename, FILE* errors) {
	token_list_p list = calloc(1, sizeof(token_list_t));
	list->src_str = src_str;
	list->src_len = src_len;
	list->filename = filename;
	
	lexer_p lexer = &(lexer_t){
		.pos    = 0,
		.tokens = list,
		.errors = errors
	};
	
	token_t t;
	do {
		t = next_token(lexer);
		
		list->tokens_len++;
		list->tokens_ptr = realloc(list->tokens_ptr, list->tokens_len * sizeof(list->tokens_ptr[0]));
		list->tokens_ptr[list->tokens_len-1] = t;
		
		if (t.type == T_ERROR)
			list->error_count++;
	} while (t.type != T_EOF);
	
	return list;
}

void lex_free(token_list_p list) {
	for(size_t i = 0; i < list->tokens_len; i++) {
		token_type_t type = list->tokens_ptr[i].type;
		if ( type == T_STR )
			free(list->tokens_ptr[i].str_val);
	}
	
	free(list->tokens_ptr);
	free(list);
}


//
// The lexer itself
//

static struct { const char* keyword; token_type_t type; } keywords[] = {
	{ "syscall", T_SYSCALL },
	{ "var", T_VAR }
};

static token_t next_token(lexer_p lexer) {
	int c = peek1(lexer);
	switch(c) {
		case EOF:
			return new_token(lexer, T_EOF, 0);
		case '{':
			return new_token(lexer, T_CBO, 1);
		case '}':
			return new_token(lexer, T_CBC, 1);
		case '(':
			return new_token(lexer, T_RBO, 1);
		case ')':
			return new_token(lexer, T_RBC, 1);
		case ',':
			return new_token(lexer, T_COMMA, 1);
		case '=':
			return new_token(lexer, T_ASSIGN, 1);
	}
	
	if ( isspace(c) ) {
		token_t t = new_token(lexer, (c == '\n') ? T_WSNL : T_WS, 1);
		
		while ( isspace(c = peek1(lexer)) ) {
			putc_into_token(lexer, &t, c);
			// If a white space token contains a new line it becomes a possible end of statement
			if (c == '\n')
				t.type = T_WSNL;
		}
		
		return t;
	}
	
	// TODO: Handle numbers starting with a "0" prefix (e.g. 0b... 0o... 0x...)
	// But also "0" itself.
	if ( isdigit(c) ) {
		token_t t = new_token(lexer, T_INT, 1);
		
		int value = c - '0';
		while ( isdigit(c = peek1(lexer)) ) {
			putc_into_token(lexer, &t, c);
			value = value * 10 + (c - '0');
		}
		
		t.int_val = value;
		return t;
	}
	
	if ( c == '/' ) {
		if ( peek2(lexer) == '/' ) {
			token_t t = new_token(lexer, T_COMMENT, 2);
			
			while ( c = peek1(lexer), !(c == '\n' || c == EOF) ) {
				putc_into_token(lexer, &t, c);
			}
			
			return t;
		} else if ( peek2(lexer) == '*' ) {
			token_t t = new_token(lexer, T_COMMENT, 2);
			
			int nesting_level = 1;
			while ( nesting_level > 0 ) {
				if ( peek1(lexer) == '*' && peek2(lexer) == '/' ) {
					nesting_level--;
					putc_into_token(lexer, &t, '*');
					putc_into_token(lexer, &t, '/');
				} else if ( peek1(lexer) == '/' && peek2(lexer) == '*' ) {
					nesting_level++;
					putc_into_token(lexer, &t, '/');
					putc_into_token(lexer, &t, '*');
				} else if ( peek1(lexer) == EOF ) {
					make_into_error_token(&t, "unterminated multiline comment");
					break;
				} else {
					c = peek1(lexer);
					putc_into_token(lexer, &t, c);
				}
			}
			
			return t;
		}
	}
	
	if (c == '"') {
		token_t t = new_token(lexer, T_STR, 1);
		
		while (true) {
			c = peek1(lexer);
			putc_into_token(lexer, &t, c);
			
			switch(c) {
				case '"':
					return t;
				default:
					append_token_str(&t, c);
					break;
					
				case EOF:
					make_into_error_token(&t, "unterminated string");
					return t;
				case '\\':
					c = peek1(lexer);
					putc_into_token(lexer, &t, c);
					switch(c) {
						case EOF:
							make_into_error_token(&t, "unterminated escape code in string");
							return t;
						case '\\':
							append_token_str(&t, '\\');
							break;
						case '"':
							append_token_str(&t, '"');
							break;
						case 'n':
							append_token_str(&t, '\n');
							break;
						case 't':
							append_token_str(&t, '\t');
							break;
						default:
							// TODO: Figure out how to create a error token that reprsents
							// a region inside another token (the string)...
							//make_into_error_token(&t, "unknown escape code in string");
							fprintf(lexer->errors, "unknown escape code in string (\\%c), ignoring FOR NOW\n", c);
							break;
					}
					break;
			}
		}
	}
	
	// Can't put / operator at the top since it would have a higher priority
	// than comments ("//"). Resulting in two division operators instead of
	// an comment.
	switch(c) {
		case '+':
		case '-':
		case '*':
		case '/':
			return new_token(lexer, T_ID, 1);
	}
	
	if ( isalpha(c) || c == '_' ) {
		token_t t = new_token(lexer, T_ID, 1);
		
		while ( c = peek1(lexer), (isalnum(c) || c == '_') )
			putc_into_token(lexer, &t, c);
		
		for(size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
			if ( strncmp(t.src_str, keywords[i].keyword, t.src_len) == 0 ) {
				t.type = keywords[i].type;
				break;
			}
		}
		
		return t;
	}
	
	// Abort on any unknown char. Ignoring them will just get us surprised...
	token_t t = new_token(lexer, T_ERROR, 1);
	make_into_error_token(&t, "stray character in source code");
	return t;
}