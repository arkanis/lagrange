#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "utils.h"
#include "asm.h"


/*

Content is ordered after when it becomes relevant for the compiler.

external modules: utils.h, asm.h
tokenization: tokenizer structs and functions
parsing: ast structs and functions
AST
operator reordering: operator table
namespace pass
type pass
linker pass
compiler: reg allocator

*/

//
// Management structs for the program
//

typedef struct token_s  token_t, *token_p;
typedef list_t(token_t) token_list_t, *token_list_p;
typedef struct node_s   node_t, *node_p;
typedef struct type_s   type_t, *type_p;

typedef struct {
	char* filename;
	str_t source;
	
	token_list_t tokens;
	
} module_t, *module_p;



//
// Tokenizer
//

#define TOKEN(id, keyword, free_expr, desc) id,
typedef enum {
	#include "token_spec.h"
} token_type_t;
#undef TOKEN

struct token_s {
	token_type_t type;
	str_t source;
	
	union {
		str_t   str_val;
		int64_t int_val;
	};
};

size_t tokenize(str_t source, token_list_p tokens, FILE* error_stream);

void token_free(token_p token);
int  token_line(module_p module, token_p token);
int  token_col(module_p module, token_p token);

#define TP_SOURCE      (1 << 0)  // print only source
#define TP_DUMP        (1 << 1)  // print type and source
#define TP_INLINE_DUMP (1 << 2)  // print type and escaped and shorted source to avoid line breaks in the output
void  token_print(FILE* stream, token_p token, uint32_t flags);

void  token_print_line(FILE* stream, module_p module, token_p token);
void  token_print_range(FILE* stream, module_p module, size_t token_start_idx, size_t token_count);
char* token_type_name(token_type_t type);
char* token_desc(token_type_t type);



//
// Parsing
//

typedef struct parser_s parser_t, *parser_p;
typedef node_p (*parser_rule_func_t)(parser_p parser);

node_p parse(module_p module, parser_rule_func_t rule, FILE* error_stream);

node_p parse_module(parser_p parser);
node_p parse_func_def(parser_p parser);
node_p parse_expr(parser_p parser);



//
// Namespaces
//

#include "slim_hash.h"

SH_GEN_DECL(node_ns, str_t, node_p);

node_p ns_lookup(node_p node, str_t name);


//
// Address slot
//

typedef struct {
	size_t offset;
	node_p target;
	// type? 32bit displ, or something else?
} node_addr_slot_t, *node_addr_slot_p;


//
// Node list
//

typedef list_t(node_p) node_list_t, *node_list_p;

void node_list_append(node_list_p list, node_p node);
void node_list_replace_n1(node_list_p list, size_t start_idx, size_t hole_len, node_p replacement_node);


//
// Types for node specs
//

typedef enum {
	MT_NODE = 1,
	MT_NODE_LIST,
	MT_NS,
	MT_ASL,
	MT_TYPE,
	
	MT_INT,
	MT_CHAR,
	MT_STR,
	MT_SIZE,
	MT_BOOL,
	
	MT_NONE
} member_type_t;

typedef struct {
	member_type_t type;
	uint32_t offset;
	char* name;
} member_spec_t, *member_spec_p;

typedef struct {
	char*         name;
	member_spec_p members;
} node_spec_t, *node_spec_p;


//
// List of module types
//

#define BEGIN(nn, NN)           NT_##NN,
#define MEMBER(nn, mn, ct, mt)  
#define END(nn)                 

typedef enum {
	#include "ast_spec.h"
} node_type_t;

#undef BEGIN
#undef MEMBER
#undef END


//
// Node definitions
//

#define BEGIN(nn, NN)           struct {
#define MEMBER(nn, mn, ct, mt)  	ct mn;
#define END(nn)                 } nn;

struct node_s {
	node_type_t type;
	node_spec_p spec;
	node_p parent;
	
	token_list_t tokens;
	
	union {
		#include "ast_spec.h"
	};
};

#undef BEGIN
#undef MEMBER
#undef END

// Actual specs are defined in ast.c
node_spec_p* node_specs;


//
// Node functions
//

node_p node_alloc(node_type_t type);
node_p node_alloc_set(node_type_t type, node_p parent, node_p* member);
node_p node_alloc_append(node_type_t type, node_p parent, node_list_p list);

void node_set(node_p parent, node_p* member, node_p child);
void node_append(node_p parent, node_list_p list, node_p child);

void node_print(node_p node, FILE* output);
void node_print_inline(node_p node, FILE* output);


// Example:
// 
// for(ast_it_t it = ast_start(node); it.node != NULL; it = ast_next(node, it))
//   foo(it.node);

typedef struct {
	ssize_t member_index;
	ssize_t node_index;
	node_p node;
} ast_it_t;

ast_it_t ast_start(node_p node);
ast_it_t ast_next(node_p node, ast_it_t it);