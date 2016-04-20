#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "lexer.h"


typedef struct {
	size_t pos;
	int line;
	tokenized_file_p file;
	FILE* errors;
	bool at_eof;
} tokenizer_t, *tokenizer_p;

static int     consume_char(tokenizer_p tokenizer);
static void    revert_char(tokenizer_p tokenizer);
static token_t new_token(tokenizer_p tokenizer, token_type_t type, int start_offset, int size);
static void    append_token_str(token_p token, char c);
static token_t next_token(tokenizer_p tokenizer);


tokenized_file_p tokenize_str(char* source, size_t size, const char* name, FILE* errors) {
	tokenized_file_p file = malloc(sizeof(tokenized_file_t));
	file->name = name;
	file->source = source;
	file->source_size = size;
	file->tokens = NULL;
	file->token_count = 0;
	
	tokenizer_t tokenizer;
	tokenizer.pos = 0;
	tokenizer.line = 1;
	tokenizer.file = file;
	tokenizer.errors = errors;
	tokenizer.at_eof = false;
	
	token_t t;
	do {
		t = next_token(&tokenizer);
		
		file->token_count++;
		file->tokens = realloc(file->tokens, file->token_count * sizeof(file->tokens[0]));
		file->tokens[file->token_count-1] = t;
	} while (t.type != T_EOF);
	
	return file;
}

void tokenized_file_free(tokenized_file_p file) {
	for(size_t i = 0; i < file->token_count; i++) {
		if (file->tokens[i].type == T_STR)
			free(file->tokens[i].str_val);
	}
	
	free(file->tokens);
	free(file);
}

int token_dump(token_p token, char* buffer, size_t buffer_size) {
	switch(token->type) {
		case T_COMMENT: snprintf(buffer, buffer_size, "comment(\"%.*s\")", token->size, token->source);  break;
		case T_WS:      snprintf(buffer, buffer_size, "ws(\"%.*s\")", token->size, token->source);       break;
		case T_WS_EOS:  snprintf(buffer, buffer_size, "ws_eos(\"%.*s\")", token->size, token->source);   break;
		case T_FUNC:    snprintf(buffer, buffer_size, "func");                                           break;
		case T_ID:      snprintf(buffer, buffer_size, "id(%.*s)", token->size, token->source);           break;
		case T_CBO:     snprintf(buffer, buffer_size, "\"{\"");                                          break;
		case T_CBC:     snprintf(buffer, buffer_size, "\"}\"");                                          break;
		case T_RBO:     snprintf(buffer, buffer_size, "\"(\"");                                          break;
		case T_RBC:     snprintf(buffer, buffer_size, "\")\"");                                          break;
		case T_COMMA:   snprintf(buffer, buffer_size, "\",\"");                                          break;
		case T_ASSIGN:  snprintf(buffer, buffer_size, "\"=\"");                                          break;
		case T_RET:     snprintf(buffer, buffer_size, "return");                                         break;
		case T_INT:     snprintf(buffer, buffer_size, "int(%d)", token->int_val);                        break;
		case T_EOF:     snprintf(buffer, buffer_size, "EOF");                                            break;
		case T_STR:     snprintf(buffer, buffer_size, "str(\"%.*s\")", token->str_size, token->str_val); break;
	}
	
	return -1;
}

void token_print_line(token_p token) {
	char* line_start = token->source;
	char* line_end = token->source;
	while (line_start > token->file->source && *(line_start-1) != '\n')
		line_start--;
	while (*line_end != '\n' && line_end < token->file->source + token->file->source_size)
		line_end++;
	printf("%.*s\n", (int)(line_end - line_start), line_start);
	
	for(char* c = line_start; c < token->source; c++) {
		if ( isspace(*c) )
			printf("%c", *c);
		else
			printf(" ");
	}
	printf("^\n");
}



//
// Local helper functions
//

static int consume_char(tokenizer_p tokenizer) {
	if (tokenizer->pos >= tokenizer->file->source_size) {
		tokenizer->at_eof = true;
		return EOF;
	}
	
	int c = tokenizer->file->source[tokenizer->pos];
	tokenizer->pos++;
	
	if (c == '\n')
		tokenizer->line++;
	
	return c;
}

static void revert_char(tokenizer_p tokenizer) {
	// Don't revert if we're at the first char or if we're at the EOF
	// Otherwise some loops (e.g. int lexing loop) will hand since it
	// allways reverts the EOF, parses one digit, reverts the EOF, etc.
	// This is the same behavour as with ungetc().
	if (tokenizer->pos == 0 || tokenizer->at_eof)
		return;
	
	int c = tokenizer->file->source[tokenizer->pos];
	tokenizer->pos--;
	
	if (c == '\n')
		tokenizer->line--;
}

static token_t new_token(tokenizer_p tokenizer, token_type_t type, int start_offset, int size) {
	return (token_t){
		.type = type,
		.line = tokenizer->line,
		.source = tokenizer->file->source + tokenizer->pos + start_offset,
		.size = size,
		.file = tokenizer->file,
		.str_size = 0,
		.str_val = NULL
	  };
}

static void append_token_str(token_p token, char c) {
	token->str_size++;
	token->str_val = realloc(token->str_val, token->str_size);
	token->str_val[token->str_size-1] = c;
}



//
// The lexer itself
//

struct { const char* keyword; token_type_t type; } keywords[] = {
	{ "function", T_FUNC },
	{ "return", T_RET }
};

token_t next_token(tokenizer_p tokenizer) {
	int c = consume_char(tokenizer);
	switch(c) {
		case EOF:
			return new_token(tokenizer, T_EOF, 0, 0);
		case '{':
			return new_token(tokenizer, T_CBO, -1, 1);
		case '}':
			return new_token(tokenizer, T_CBC, -1, 1);
		case '(':
			return new_token(tokenizer, T_RBO, -1, 1);
		case ')':
			return new_token(tokenizer, T_RBC, -1, 1);
		case ',':
			return new_token(tokenizer, T_COMMA, -1, 1);
		case '=':
			return new_token(tokenizer, T_ASSIGN, -1, 1);
	}
	
	if ( isspace(c) ) {
		token_t t = new_token(tokenizer, T_WS, -1, 0);
		
		do {
			t.size++;
			// If a white space token contains a new line it becomes a possible end of statement
			if (c == '\n')
				t.type = T_WS_EOS;
			c = consume_char(tokenizer);
		} while ( isspace(c) );
		revert_char(tokenizer);
		
		return t;
	}
	
	if ( c != '0' && isdigit(c) ) {
		token_t t = new_token(tokenizer, T_INT, -1, 1);
		
		int value = 0;
		do {
			value = value * 10 + (c - '0');
			c = consume_char(tokenizer);
			t.size++;
		} while ( isdigit(c) );
		revert_char(tokenizer);
		t.size--;
		
		t.int_val = value;
		return t;
	}
	
	if ( c == '/' ) {
		int old_c = c;
		c = consume_char(tokenizer);
		
		if (c == '/') {
			token_t t = new_token(tokenizer, T_COMMENT, -2, 1);
			
			do {
				t.size++;
				c = consume_char(tokenizer);
			} while ( !(c == '\n' || c == EOF) );
			revert_char(tokenizer);
			t.size--;
			
			return t;
		} else if (c == '*') {
			token_t t = new_token(tokenizer, T_COMMENT, -2, 2);
			
			int nesting_level = 1;
			while ( nesting_level > 0 ) {
				c = consume_char(tokenizer);
				t.size++;
				
				if (c == '*') {
					c = consume_char(tokenizer);
					t.size++;
					
					if (c == '/') {
						nesting_level--;
					}
				} else if (c == '/') {
					c = consume_char(tokenizer);
					t.size++;
					
					if (c == '*') {
						nesting_level++;
					}
				}
			}
			
			return t;
		}
		
		// Not a comment, put the peeked char back into stream so the / will be matched as an T_ID
		revert_char(tokenizer);
		c = old_c;
	}
	
	if (c == '"') {
		token_t t = new_token(tokenizer, T_STR, -1, 1);
		
		while(1) {
			c = consume_char(tokenizer);
			t.size++;
			switch(c) {
				case EOF:
					fprintf(tokenizer->errors, "unterminated string!\n");
					return t;
				case '"':
					return t;
				case '\\':
					c = consume_char(tokenizer);
					t.size++;
					switch(c) {
						case EOF:
							fprintf(tokenizer->errors, "unterminated escape code in string!\n");
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
							fprintf(tokenizer->errors, "unknown escape code in string: \\%c, ignoring\n", c);
							break;
					}
					break;
				default:
					append_token_str(&t, c);
					break;
			}
		}
	}
	
	if ( isalpha(c) || c == '+' || c == '-' || c == '*' || c == '/' || c == '_' ) {
		token_t t = new_token(tokenizer, T_ID, -1, 1);
		
		do {
			t.size++;
			c = consume_char(tokenizer);
		} while ( isalnum(c) || c == '_' );
		revert_char(tokenizer);
		t.size--;
		
		for(size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
			if ( strncmp(t.source, keywords[i].keyword, t.size) == 0 ) {
				t.type = keywords[i].type;
				break;
			}
		}
		
		return t;
	}
	
	// Abort on any unknown char. Ignoring them will just get us surprised...
	fprintf(tokenizer->errors, "Unknown character: '%c' (%d), aborting\n", c, c);
	abort();
	return (token_t){0};
	
	//fprintf(tokenizer->errors, "Unknown character: '%c' (%d), ignoring\n", c, c);
	//return next_token(tokenizer);
}