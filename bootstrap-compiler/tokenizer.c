#include <ctype.h>
#include <string.h>
#include "common.h"


typedef struct {
	str_t        source;
	size_t       pos;
	token_list_p tokens;
	FILE*        error_stream;
	size_t       error_count;
} tokenizer_ctx_t, *tokenizer_ctx_p;


//
// Keyword to token type mapping
//
static struct { const char* keyword; token_type_t type; } keywords[] = {
	#define _ NULL
	#define TOKEN(id, keyword, ...) { keyword, id },
	#include "token_spec.h"
	#undef TOKEN
	#undef _
};


//
// Private tokenizer support functions
//

static int peek_at_offset(tokenizer_ctx_p ctx, size_t offset) {
	if (ctx->pos + offset >= (size_t)ctx->source.len)
		return EOF;
	return ctx->source.ptr[ctx->pos + offset];
}

static int peek1(tokenizer_ctx_p ctx) {
	return peek_at_offset(ctx, 0);
}

static int peek2(tokenizer_ctx_p ctx) {
	return peek_at_offset(ctx, 1);
}

static int peek3(tokenizer_ctx_p ctx) {
	return peek_at_offset(ctx, 2);
}

static token_t new_token(tokenizer_ctx_p ctx, token_type_t type, int chars_to_consume) {
	if (ctx->pos + chars_to_consume > (size_t)ctx->source.len) {
		fprintf(ctx->error_stream, "Tried to consume a char beyond EOF!\n");
		abort();
	}
	
	token_t token = (token_t){
		.type   = type,
		.source = str_from_mem(ctx->source.ptr + ctx->pos, chars_to_consume)
	};
	ctx->pos += chars_to_consume;
	return token;
}

static void consume_into_token(tokenizer_ctx_p ctx, token_p token, int chars_to_consume) {
	if (ctx->pos + chars_to_consume > (size_t)ctx->source.len) {
		fprintf(ctx->error_stream, "Tried to consume a char beyond EOF!\n");
		abort();
	} else if (ctx->source.ptr + ctx->pos != token->source.ptr + token->source.len) {
		fprintf(ctx->error_stream, "Tried to put a char into a token thats end isn't the current tokenizer position!\n");
		abort();
	}
	
	ctx->pos          += chars_to_consume;
	token->source.len += chars_to_consume;
}

static void append_token(tokenizer_ctx_p ctx, token_t token) {
	list_append(ctx->tokens, token);
}

static void make_into_error_token(tokenizer_ctx_p ctx, token_p token, char* message) {
	ctx->error_count++;
	
	token->type = T_ERROR;
	token->str_val = str_from_c(message);
}

static token_t new_error_token(tokenizer_ctx_p ctx, ssize_t start_offset, size_t length, char* message) {
	if ( (ssize_t)ctx->pos + start_offset < 0 || ctx->pos + start_offset + length > (size_t)ctx->source.len ) {
		fprintf(ctx->error_stream, "The error token contains bytes outside of the source string!\n");
		abort();
	}
	
	ctx->error_count++;
	
	return (token_t){
		.type    = T_ERROR,
		.source  = str_from_mem(ctx->source.ptr + ctx->pos + start_offset, length),
		.str_val = str_from_c(message)
	};
}



//
// Tokenizer
//

static bool next_token(tokenizer_ctx_p ctx);
static void tokenize_one_line_comment(tokenizer_ctx_p ctx);
static void tokenize_nested_multiline_comment(tokenizer_ctx_p ctx);
static void tokenize_string(tokenizer_ctx_p ctx);

size_t tokenize(str_t source, token_list_p tokens, FILE* error_stream) {
	tokenizer_ctx_t ctx = (tokenizer_ctx_t){
		.source = source,
		.pos    = 0,
		.tokens = tokens,
		.error_stream = error_stream,
		.error_count  = 0
	};
	
	while ( next_token(&ctx) ) { }
	
	return ctx.error_count;
}

static bool next_token(tokenizer_ctx_p ctx) {
	int c = peek1(ctx);
	int c2 = peek2(ctx);
	int c3 = peek3(ctx);
	switch(c) {
		case EOF:  append_token(ctx, new_token(ctx, T_EOF, 0));     return false;
		case '{':  append_token(ctx, new_token(ctx, T_CBO, 1));     return true;
		case '}':  append_token(ctx, new_token(ctx, T_CBC, 1));     return true;
		case '(':  append_token(ctx, new_token(ctx, T_RBO, 1));     return true;
		case ')':  append_token(ctx, new_token(ctx, T_RBC, 1));     return true;
		case '[':  append_token(ctx, new_token(ctx, T_SBO, 1));     return true;
		case ']':  append_token(ctx, new_token(ctx, T_SBC, 1));     return true;
		case ',':  append_token(ctx, new_token(ctx, T_COMMA, 1));   return true;
		case '.':  append_token(ctx, new_token(ctx, T_PERIOD, 1));  return true;
		case ';':  append_token(ctx, new_token(ctx, T_SEMI, 1));    return true;
		case ':':  append_token(ctx, new_token(ctx, T_COLON, 1));   return true;
		case '~':  append_token(ctx, new_token(ctx, T_COMPL, 1));   return true;
		case '"':  tokenize_string(ctx);  return true;
		
		case '+':
			switch(c2) {
				case '=':  append_token(ctx, new_token(ctx, T_ADD_ASSIGN, 2));  return true;
				default:   append_token(ctx, new_token(ctx, T_ADD, 1));         return true;
			}
		case '-':
			switch(c2) {
				case '=':  append_token(ctx, new_token(ctx, T_SUB_ASSIGN, 2));  return true;
				default:   append_token(ctx, new_token(ctx, T_SUB, 1));         return true;
			}
		case '*':
			switch(c2) {
				case '=':  append_token(ctx, new_token(ctx, T_MUL_ASSIGN, 2));  return true;
				default:   append_token(ctx, new_token(ctx, T_MUL, 1));         return true;
			}
		case '/':
			switch(c2) {
				case '/':  tokenize_one_line_comment(ctx);                      return true;
				case '*':  tokenize_nested_multiline_comment(ctx);              return true;
				case '=':  append_token(ctx, new_token(ctx, T_DIV_ASSIGN, 2));  return true;
				default:   append_token(ctx, new_token(ctx, T_DIV, 1));         return true;
			}
		case '%':
			switch(c2) {
				case '=':  append_token(ctx, new_token(ctx, T_MOD_ASSIGN, 2));  return true;
				default:   append_token(ctx, new_token(ctx, T_MOD, 1));         return true;
			}
		
		case '<':
			switch(c2) {
				case '<':
					switch(c3) {
						case '=':  append_token(ctx, new_token(ctx, T_SL_ASSIGN, 3));  return true;
						default:   append_token(ctx, new_token(ctx, T_SL, 2));         return true;
					}
				case '=':  append_token(ctx, new_token(ctx, T_LE, 2));  return true;
				default:   append_token(ctx, new_token(ctx, T_LT, 1));  return true;
			}
		case '>':
			switch(c2) {
				case '>':
					switch(c3) {
						case '=':  append_token(ctx, new_token(ctx, T_SR_ASSIGN, 3));  return true;
						default:   append_token(ctx, new_token(ctx, T_SR, 2));         return true;
					}
				case '=':  append_token(ctx, new_token(ctx, T_GE, 2));  return true;
				default:   append_token(ctx, new_token(ctx, T_GT, 1));  return true;
			}
		
		case '&':
			switch(c2) {
				case '&':  append_token(ctx, new_token(ctx, T_AND, 2));             return true;
				case '=':  append_token(ctx, new_token(ctx, T_BIT_AND_ASSIGN, 2));  return true;
				default:   append_token(ctx, new_token(ctx, T_BIT_AND, 1));         return true;
			}
		case '|':
			switch(c2) {
				case '|':  append_token(ctx, new_token(ctx, T_OR, 2));             return true;
				case '=':  append_token(ctx, new_token(ctx, T_BIT_OR_ASSIGN, 2));  return true;
				default:   append_token(ctx, new_token(ctx, T_BIT_OR, 1));         return true;
			}
		case '^':
			switch(c2) {
				case '=':  append_token(ctx, new_token(ctx, T_BIT_XOR_ASSIGN, 2));  return true;
				default:   append_token(ctx, new_token(ctx, T_BIT_XOR, 1));         return true;
			}
		
		case '=':
			switch(c2) {
				case '=':  append_token(ctx, new_token(ctx, T_EQ, 2));      return true;
				default:   append_token(ctx, new_token(ctx, T_ASSIGN, 1));  return true;
			}
		case '!':
			switch(c2) {
				case '=':  append_token(ctx, new_token(ctx, T_NEQ, 2));  return true;
				default:   append_token(ctx, new_token(ctx, T_NOT, 1));  return true;
			}
	}
	
	if ( isspace(c) ) {
		token_t t = new_token(ctx, (c == '\n') ? T_WSNL : T_WS, 1);
		
		while ( isspace(c = peek1(ctx)) ) {
			consume_into_token(ctx, &t, 1);
			// If a white space token contains a new line it becomes a possible end of statement
			if (c == '\n')
				t.type = T_WSNL;
		}
		
		append_token(ctx, t);
		return true;
	}
	
	// TODO: Handle numbers starting with a "0" prefix (e.g. 0b... 0o... 0x...)
	// But also "0" itself.
	if ( isdigit(c) ) {
		token_t t = new_token(ctx, T_INT, 1);
		
		int value = c - '0';
		while ( isdigit(c = peek1(ctx)) ) {
			consume_into_token(ctx, &t, 1);
			value = value * 10 + (c - '0');
		}
		
		t.int_val = value;
		append_token(ctx, t);
		return true;
	}
	
	if ( isalpha(c) || c == '_' ) {
		token_t t = new_token(ctx, T_ID, 1);
		
		while ( c = peek1(ctx), (isalnum(c) || c == '_') )
			consume_into_token(ctx, &t, 1);
		
		for(size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
			if (
				keywords[i].keyword != NULL &&
				strncmp(t.source.ptr, keywords[i].keyword, t.source.len) == 0 &&
				(int)strlen(keywords[i].keyword) == t.source.len
			) {
				t.type = keywords[i].type;
				break;
			}
		}
		
		append_token(ctx, t);
		return true;
	}
	
	// Abort on any unknown char. Ignoring them will just get us surprised...
	token_t t = new_token(ctx, T_ERROR, 1);
	make_into_error_token(ctx, &t, "stray character in source code");
	append_token(ctx, t);
	return true;
}

// Function is called when "//" was peeked. So it's safe to consume 2 chars
// right away.
static void tokenize_one_line_comment(tokenizer_ctx_p ctx) {
	token_t t = new_token(ctx, T_COMMENT, 2);
	
	int c;
	while ( c = peek1(ctx), !(c == '\n' || c == EOF) )
		consume_into_token(ctx, &t, 1);
	
	append_token(ctx, t);
	return;
}

// Function is called when "/*" was peeked. So it's safe to consume 2 chars
// right away.
static void tokenize_nested_multiline_comment(tokenizer_ctx_p ctx) {
	token_t t = new_token(ctx, T_COMMENT, 2);
	
	int nesting_level = 1;
	while ( nesting_level > 0 ) {
		if ( peek1(ctx) == '*' && peek2(ctx) == '/' ) {
			nesting_level--;
			consume_into_token(ctx, &t, 2);
		} else if ( peek1(ctx) == '/' && peek2(ctx) == '*' ) {
			nesting_level++;
			consume_into_token(ctx, &t, 2);
		} else if ( peek1(ctx) == EOF ) {
			make_into_error_token(ctx, &t, "unterminated multiline comment");
			break;
		} else {
			consume_into_token(ctx, &t, 1);
		}
	}
	
	append_token(ctx, t);
	return;
}

// Function is called when '"' was peeked. So it's safe to consume one char
// right away.
static void tokenize_string(tokenizer_ctx_p ctx) {
	token_t t = new_token(ctx, T_STR, 1);
	
	while (true) {
		int c = peek1(ctx);
		if (c != EOF)
			consume_into_token(ctx, &t, 1);
		
		switch(c) {
			case '"':
				append_token(ctx, t);
				return;
			default:
				str_putc(&t.str_val, c);
				break;
				
			case EOF:
				make_into_error_token(ctx, &t, "unterminated string");
				append_token(ctx, t);
				return;
			case '\\':
				c = peek1(ctx);
				if (c != EOF)
					consume_into_token(ctx, &t, 1);
				
				switch(c) {
					case EOF:
						make_into_error_token(ctx, &t, "unterminated escape code in string");
						append_token(ctx, t);
						return;
					case '\\':
						str_putc(&t.str_val, '\\');
						break;
					case '"':
						str_putc(&t.str_val, '"');
						break;
					case 'n':
						str_putc(&t.str_val, '\n');
						break;
					case 't':
						str_putc(&t.str_val, '\t');
						break;
					default:
						// Just report the invalid escape code as error token
						append_token(ctx, new_error_token(ctx, -2, 2, "unknown escape code in string"));
						break;
				}
				break;
		}
	}
}



//
// Utility functions
//

void token_free(token_p token) {
	token_p t = token;
	switch(token->type) {
		#define _
		#define TOKEN(id, keyword, free_expr, ...) case id: free_expr; break;
		#include "token_spec.h"
		#undef TOKEN
		#undef _
	}
}

int token_line(module_p module, token_p token) {
	int line = 1;
	for(char* c = token->source.ptr; c >= module->source.ptr; c--) {
		if (*c == '\n')
			line++;
	}
	return line;
}

int token_col(module_p module, token_p token) {
	int col = 0;
	for(char* c = token->source.ptr; *c != '\n' && c >= module->source.ptr; c--)
		col++;
	return col;
}



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
		fprintf(stream, "%.*s", token->source.len, token->source.ptr);
	} else if ( (flags & TP_DUMP) || (flags & TP_INLINE_DUMP) ) {
		// Handle tokens with variable content
		switch(token->type) {
			case T_COMMENT:
				fprintf(stream, "comment(");
				if (flags & TP_DUMP)
					fprintf(stream, "%.*s", token->source.len, token->source.ptr);
				else
					token_print_escaped_str(stream, token->source);
				fprintf(stream, ")");
				return;
			case T_WS:
				fprintf(stream, "ws(\"");
				if (flags & TP_DUMP)
					fprintf(stream, "%.*s", token->source.len, token->source.ptr);
				else
					token_print_escaped_str(stream, token->source);
				fprintf(stream, "\")");
				return;
			case T_WSNL:
				fprintf(stream, "wsnl(\"");
				if (flags & TP_DUMP)
					fprintf(stream, "%.*s", token->source.len, token->source.ptr);
				else
					token_print_escaped_str(stream, token->source);
				fprintf(stream, "\")");
				return;
			
			case T_STR:
				fprintf(stream, "str(\"");
				if (flags & TP_DUMP)
					fprintf(stream, "%.*s", token->str_val.len, token->str_val.ptr);
				else
					token_print_escaped_str(stream, token->str_val);
				fprintf(stream, "\")");
				return;
			case T_INT:     fprintf(stream, "int(%ld)",    token->int_val);                          return;
			case T_ID:      fprintf(stream, "id(%.*s)",    token->source.len, token->source.ptr);    return;
			case T_ERROR:   fprintf(stream, "error(%.*s)", token->str_val.len, token->str_val.ptr);  return;
			
			default:
				break;
		}
		
		// Generate code for tokens that simply print a string
		switch(token->type) {
			#define _ NULL
			// if(desc) fputs(desc, stream) doesn't work because of a compile time check that the first
			// argument of fputs can't be NULL. Even if the if statement makes sure it isn't.
			#define TOKEN(id, keyword, free_expr, desc, ...) case id: fputs(desc ? desc : "", stream); return;
			#include "token_spec.h"
			#undef TOKEN
			#undef _
			default:
				break;
		}
		
		fprintf(stderr, "token_print(): Unhandled token type! Please update the token spec or add printing code.\n");
		abort();
	}
}

void token_print_line(FILE* stream, module_p module, token_p token) {
	if (token < module->tokens.ptr || token > module->tokens.ptr + module->tokens.len) {
		fprintf(stderr, "token_print_line(): Specified token isn't part of the modules token list!\n");
		abort();
	}
	
	size_t index = token - module->tokens.ptr;
	token_print_range(stream, module, index, 1);
}

void token_print_range(FILE* stream, module_p module, size_t token_start_idx, size_t token_count) {
	token_p start_token = &module->tokens.ptr[token_start_idx];
	token_p end_token = &module->tokens.ptr[token_start_idx + token_count - 1];
	
	char* code_start = start_token->source.ptr;
	char* code_end = end_token->source.ptr + end_token->source.len;
	
	char* line_start = code_start;
	while (line_start > module->source.ptr && *(line_start-1) != '\n')
		line_start--;
	char* line_end = code_end;
	while (*line_end != '\n' && line_end < module->source.ptr + module->source.len)
		line_end++;
	
	fprintf(stream, "%.*s\e[1;4m%.*s\e[0m%.*s\n",
		(int)(code_start - line_start), line_start,
		(int)(code_end - code_start),   code_start,
		(int)(line_end - code_end),     code_end
	);
}


char* token_type_names[] = {
	#define TOKEN(id, keyword, free_expr, desc) #id,
	#include "token_spec.h"
	#undef TOKEN
};

char* token_type_name(token_type_t type) {
	return token_type_names[type];
}

char* token_descs[] = {
	#define _ NULL
	#define TOKEN(id, keyword, free_expr, desc, ...) desc,
	#include "token_spec.h"
	#undef TOKEN
	#undef _
};

char* token_desc(token_type_t type) {
	return token_descs[type];
}