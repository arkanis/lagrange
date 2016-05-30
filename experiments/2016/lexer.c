#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "lexer.h"


//
// Token printing functions
//

static void token_print_escaped_str(FILE* stream, const str_t str) {
	for(int i = 0; i < str.len && i < 20 && str.ptr[i] != '\0'; i++) {
		switch(str.ptr[i]) {
			case '\n':  fprintf(stream, "\\n");    break;
			case '\t':  fprintf(stream, "\\t");    break;
			default:    putc(str.ptr[i], stream);  break;
		}
	}
}

void token_print(FILE* stream, token_p token, uint32_t flags) {
	if (flags & TP_SOURCE) {
		fprintf(stream, "%.*s", token->src.len, token->src.ptr);
	} else if ( (flags & TP_DUMP) || (flags & TP_INLINE_DUMP) ) {
		switch(token->type) {
			case T_COMMENT:
				fprintf(stream, "comment(");
				if (flags & TP_DUMP)
					fprintf(stream, "%.*s", token->src.len, token->src.ptr);
				else
					token_print_escaped_str(stream, token->src);
				fprintf(stream, ")");
				break;
			case T_WS:
				fprintf(stream, "ws(\"");
				if (flags & TP_DUMP)
					fprintf(stream, "%.*s", token->src.len, token->src.ptr);
				else
					token_print_escaped_str(stream, token->src);
				fprintf(stream, "\")");
				break;
			case T_WSNL:
				fprintf(stream, "ws_eos(\"");
				if (flags & TP_DUMP)
					fprintf(stream, "%.*s", token->src.len, token->src.ptr);
				else
					token_print_escaped_str(stream, token->src);
				fprintf(stream, "\")");
				break;
			
			case T_STR:
				fprintf(stream, "str(\"");
				if (flags & TP_DUMP)
					fprintf(stream, "%.*s", token->str_val.len, token->str_val.ptr);
				else
					token_print_escaped_str(stream, token->str_val);
				fprintf(stream, "\")");
				break;
			case T_INT:     fprintf(stream, "int(%ld)", token->int_val);                  break;
			case T_ID:      fprintf(stream, "id(%.*s)", token->src.len, token->src.ptr);  break;
			
			case T_CBO:     fprintf(stream, "\"{\"");  break;
			case T_CBC:     fprintf(stream, "\"}\"");  break;
			case T_RBO:     fprintf(stream, "\"(\"");  break;
			case T_RBC:     fprintf(stream, "\")\"");  break;
			case T_COMMA:   fprintf(stream, "\",\"");  break;
			
			case T_ADD:        fprintf(stream, "\"+\"");   break;
			case T_ADD_ASSIGN: fprintf(stream, "\"+=\"");  break;
			case T_SUB:        fprintf(stream, "\"-\"");   break;
			case T_SUB_ASSIGN: fprintf(stream, "\"-=\"");  break;
			case T_MUL:        fprintf(stream, "\"*\"");   break;
			case T_MUL_ASSIGN: fprintf(stream, "\"*=\"");  break;
			case T_DIV:        fprintf(stream, "\"/\"");   break;
			case T_DIV_ASSIGN: fprintf(stream, "\"/=\"");  break;
			case T_MOD:        fprintf(stream, "\"%%\"");  break;
			case T_MOD_ASSIGN: fprintf(stream, "\"%%=\""); break;
			
			case T_LT:        fprintf(stream, "\"<\"");   break;
			case T_LE:        fprintf(stream, "\"<=\"");  break;
			case T_SL:        fprintf(stream, "\"<<\"");  break;
			case T_SL_ASSIGN: fprintf(stream, "\"<<=\""); break;
			case T_GT:        fprintf(stream, "\">\"");   break;
			case T_GE:        fprintf(stream, "\">=\"");  break;
			case T_SR:        fprintf(stream, "\">>\"");  break;
			case T_SR_ASSIGN: fprintf(stream, "\">>=\""); break;
			
			case T_BIN_AND:        fprintf(stream, "\"&\"");  break;
			case T_BIN_AND_ASSIGN: fprintf(stream, "\"&=\""); break;
			case T_BIN_XOR:        fprintf(stream, "\"^\"");  break;
			case T_BIN_XOR_ASSIGN: fprintf(stream, "\"^=\""); break;
			case T_BIN_OR:         fprintf(stream, "\"|\"");  break;
			case T_BIN_OR_ASSIGN:  fprintf(stream, "\"|=\""); break;
			
			case T_ASSIGN: fprintf(stream, "\"=\"");  break;
			case T_EQ:     fprintf(stream, "\"==\""); break;
			case T_NEQ:    fprintf(stream, "\"!=\""); break;
			case T_PERIOD: fprintf(stream, "\".\"");  break;
			case T_COMPL:  fprintf(stream, "\"~\"");  break;
			
			case T_NOT:     fprintf(stream, "not");      break;
			case T_AND:     fprintf(stream, "and");      break;
			case T_OR:      fprintf(stream, "or");       break;
			case T_SYSCALL: fprintf(stream, "syscall");  break;
			case T_VAR:     fprintf(stream, "var");      break;
			case T_IF:      fprintf(stream, "if");       break;
			case T_THEN:    fprintf(stream, "then");     break;
			case T_ELSE:    fprintf(stream, "else");     break;
			case T_WHILE:   fprintf(stream, "while");    break;
			case T_DO:      fprintf(stream, "do");       break;
			
			case T_ERROR:   fprintf(stream, "error(%.*s)", token->str_val.len, token->str_val.ptr);  break;
			case T_EOF:     fprintf(stream, "EOF");                                                  break;
		}
	}
}

void token_print_line(FILE* stream, token_p token, int first_line_indent) {
	char* line_start = token->src.ptr;
	while (line_start > token->list->src.ptr && *(line_start-1) != '\n')
		line_start--;
	char* line_end = token->src.ptr + token->src.len;
	while (*line_end != '\n' && line_end < token->list->src.ptr + token->list->src.len)
		line_end++;
	
	fprintf(stream, "%.*s\e[1;4m%.*s\e[0m%.*s\n",
		(int)(token->src.ptr - line_start), line_start,
		token->src.len, token->src.ptr,
		(int)(line_end - (token->src.ptr + token->src.len)), token->src.ptr + token->src.len
	);
}

int token_line(token_p token) {
	int line = 1;
	for(char* c = token->src.ptr; c >= token->list->src.ptr; c--) {
		if (*c == '\n')
			line++;
	}
	return line;
}

int token_col(token_p token) {
	int col = 0;
	for(char* c = token->src.ptr; *c != '\n' && c >= token->list->src.ptr; c--)
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
	if (lexer->pos + offset >= (size_t)lexer->tokens->src.len)
		return EOF;
	return lexer->tokens->src.ptr[lexer->pos + offset];
}

static int peek1(lexer_p lexer) {
	return peek_at_offset(lexer, 0);
}

static int peek2(lexer_p lexer) {
	return peek_at_offset(lexer, 1);
}

static int peek3(lexer_p lexer) {
	return peek_at_offset(lexer, 1);
}

static token_t new_token(lexer_p lexer, token_type_t type, int chars_to_consume) {
	if (lexer->pos + chars_to_consume > (size_t)lexer->tokens->src.len) {
		fprintf(lexer->errors, "Tried to consume a char beyond EOF!\n");
		abort();
	}
	
	token_t token = (token_t){
		.list = lexer->tokens,
		
		.type = type,
		.src  = { chars_to_consume, lexer->tokens->src.ptr + lexer->pos },
		
		.str_val = { 0, NULL }
	};
	lexer->pos += chars_to_consume;
	return token;
}

static void putc_into_token(lexer_p lexer, token_p token, int c) {
	// Ignore any EOF that are put into a token
	// TODO: Go idea? Might better abort() to catch errors and enless loops more easier.
	if (c == EOF)
		return;
	
	if (lexer->pos + 1 > (size_t)lexer->tokens->src.len) {
		fprintf(lexer->errors, "Tried to consume a char beyond EOF!\n");
		abort();
	} else if (lexer->tokens->src.ptr + lexer->pos != token->src.ptr + token->src.len) {
		fprintf(lexer->errors, "Tried to put a char into a token thats end isn't the current lexer position!\n");
		abort();
	} else if (lexer->tokens->src.ptr[lexer->pos] != c) {
		fprintf(lexer->errors, "Tried to consume a char ('%c') that's not the current char ('%c')\n", c, lexer->tokens->src.ptr[lexer->pos]);
		abort();
	}
	
	lexer->pos++;
	token->src.len++;
}

static void append_token_str(token_p token, char c) {
	token->str_val.len++;
	token->str_val.ptr = realloc(token->str_val.ptr, token->str_val.len);
	token->str_val.ptr[token->str_val.len-1] = c;
}

static void make_into_error_token(token_p token, char* message) {
	token->type = T_ERROR;
	token->str_val.ptr = message;
	token->str_val.len = strlen(message);
}


//
// Public lexer functions
//

static token_t next_token(lexer_p lexer);

token_list_p lex_str(str_t code, const char* filename, FILE* errors) {
	token_list_p list = calloc(1, sizeof(token_list_t));
	list->src = code;
	list->filename = filename;
	
	lexer_p lexer = &(lexer_t){
		.pos    = 0,
		.tokens = list,
		.errors = errors
	};
	
	token_t t;
	do {
		t = next_token(lexer);
		list_append(&list->tokens, t);
		
		if (t.type == T_ERROR)
			list->error_count++;
	} while (t.type != T_EOF);
	
	return list;
}

void lex_free(token_list_p list) {
	for(size_t i = 0; i < list->tokens.len; i++) {
		token_type_t type = list->tokens.ptr[i].type;
		if ( type == T_STR || type == T_ERROR )
			free(list->tokens.ptr[i].str_val.ptr);
	}
	
	free(list->tokens.ptr);
	free(list);
}


//
// The lexer itself
//

// Function is called when "//" was peeked. So it's safe to consume 2 chars
// right away.
static token_t lex_one_line_comment(lexer_p lexer) {
	token_t t = new_token(lexer, T_COMMENT, 2);
	
	int c;
	while ( c = peek1(lexer), !(c == '\n' || c == EOF) ) {
		putc_into_token(lexer, &t, c);
	}
	
	return t;
}

// Function is called when "/*" was peeked. So it's safe to consume 2 chars
// right away.
static token_t lex_nested_multiline_comment(lexer_p lexer) {
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
			int c = peek1(lexer);
			putc_into_token(lexer, &t, c);
		}
	}
	
	return t;
}

// Function is called when '"' was peeked. So it's safe to consume one char
// right away.
static token_t lex_string(lexer_p lexer) {
	token_t t = new_token(lexer, T_STR, 1);
	
	while (true) {
		int c = peek1(lexer);
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
	}}


static struct { const char* keyword; token_type_t type; } keywords[] = {
	{ "or", T_OR },
	{ "and", T_AND },
	{ "not", T_NOT },
	
	{ "syscall", T_SYSCALL },
	{ "var",  T_VAR },
	{ "if",   T_IF },
	{ "then", T_THEN },
	{ "else", T_ELSE },
	{ "while", T_WHILE },
	{ "do", T_DO },
};

static token_t next_token(lexer_p lexer) {
	int c = peek1(lexer);
	int c2 = peek2(lexer);
	int c3 = peek3(lexer);
	switch(c) {
		case EOF:  return new_token(lexer, T_EOF, 0);
		case '{':  return new_token(lexer, T_CBO, 1);
		case '}':  return new_token(lexer, T_CBC, 1);
		case '(':  return new_token(lexer, T_RBO, 1);
		case ')':  return new_token(lexer, T_RBC, 1);
		case ',':  return new_token(lexer, T_COMMA, 1);
		case '.':  return new_token(lexer, T_PERIOD, 1);
		case '~':  return new_token(lexer, T_COMPL, 1);
		case '"':  return lex_string(lexer);
		
		case '+':
			switch(c2) {
				case '=':  return new_token(lexer, T_ADD_ASSIGN, 2);
				default:   return new_token(lexer, T_ADD, 1);
			}
		case '-':
			switch(c2) {
				case '=':  return new_token(lexer, T_SUB_ASSIGN, 2);
				default:   return new_token(lexer, T_SUB, 1);
			}
		case '*':
			switch(c2) {
				case '=':  return new_token(lexer, T_MUL_ASSIGN, 2);
				default:   return new_token(lexer, T_MUL, 1);
			}
		case '/':
			switch(c2) {
				case '/':  return lex_one_line_comment(lexer);
				case '*':  return lex_nested_multiline_comment(lexer);
				case '=':  return new_token(lexer, T_DIV_ASSIGN, 2);
				default:   return new_token(lexer, T_DIV, 1);
			}
		case '%':
			switch(c2) {
				case '=':  return new_token(lexer, T_MOD_ASSIGN, 2);
				default:   return new_token(lexer, T_MOD, 1);
			}
		
		case '<':
			switch(c2) {
				case '<':
					switch(c3) {
						case '=':  return new_token(lexer, T_SL_ASSIGN, 3);
						default:   return new_token(lexer, T_SL, 2);
					}
				case '=':  return new_token(lexer, T_LE, 2);
				default:   return new_token(lexer, T_LT, 1);
			}
		case '>':
			switch(c2) {
				case '>':
					switch(c3) {
						case '=':  return new_token(lexer, T_SR_ASSIGN, 3);
						default:   return new_token(lexer, T_SR, 2);
					}
				case '=':  return new_token(lexer, T_GE, 2);
				default:   return new_token(lexer, T_GT, 1);
			}
		
		case '&':
			switch(c2) {
				case '&':  return new_token(lexer, T_AND, 2);
				case '=':  return new_token(lexer, T_BIN_AND_ASSIGN, 2);
				default:   return new_token(lexer, T_BIN_AND, 1);
			}
		case '|':
			switch(c2) {
				case '|':  return new_token(lexer, T_OR, 2);
				case '=':  return new_token(lexer, T_BIN_OR_ASSIGN, 2);
				default:   return new_token(lexer, T_BIN_OR, 1);
			}
		case '^':
			switch(c2) {
				case '=':  return new_token(lexer, T_BIN_XOR_ASSIGN, 2);
				default:   return new_token(lexer, T_BIN_XOR, 1);
			}
		
		case '=':
			switch(c2) {
				case '=':  return new_token(lexer, T_EQ, 2);
				default:   return new_token(lexer, T_ASSIGN, 1);
			}
		case '!':
			switch(c2) {
				case '=':  return new_token(lexer, T_NEQ, 2);
				default:   return new_token(lexer, T_NOT, 1);
			}
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
	
	if ( isalpha(c) || c == '_' ) {
		token_t t = new_token(lexer, T_ID, 1);
		
		while ( c = peek1(lexer), (isalnum(c) || c == '_') )
			putc_into_token(lexer, &t, c);
		
		for(size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
			if ( strncmp(t.src.ptr, keywords[i].keyword, t.src.len) == 0 ) {
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