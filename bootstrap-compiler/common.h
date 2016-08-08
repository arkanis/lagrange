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

typedef struct token_s  token_t,      *token_p;
typedef list_t(token_t) token_list_t, *token_list_p;
typedef struct node_s   node_t,       *node_p;
typedef list_t(node_p)  node_list_t,  *node_list_p;
typedef struct type_s   type_t,       *type_p;



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

void token_cleanup(token_p token);
int  token_line(node_p module, token_p token);
int  token_col(node_p module, token_p token);

#define TP_SOURCE      (1 << 0)  // print only source
#define TP_DUMP        (1 << 1)  // print type and source
#define TP_INLINE_DUMP (1 << 2)  // print type and escaped and shorted source to avoid line breaks in the output
void  token_print(FILE* stream, token_p token, uint32_t flags);

void  token_print_line(FILE* stream, node_p module, token_p token);
void  token_print_range(FILE* stream, node_p module, size_t token_start_idx, size_t token_count);
char* token_type_name(token_type_t type);
char* token_desc(token_type_t type);



//
// Parsing
//

typedef struct parser_s parser_t, *parser_p;
typedef node_p (*parser_rule_func_t)(parser_p parser);

// Takes the modules tokens and puts the result nodes into module.body.
// The rule NULL parses an entire module not just a part of the grammar.
void parse(node_p module, parser_rule_func_t rule, FILE* error_stream);

node_p parse_func_def(parser_p parser);
node_p parse_op_def(parser_p parser);
node_p parse_stmt(parser_p parser);
node_p parse_expr(parser_p parser);
node_p parse_cexpr(parser_p parser);



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

void node_list_replace_range_with_one(node_list_p list, size_t start_idx, size_t hole_len, node_p replacement_node);


//
// Types for node specs
//

typedef enum {
	MT_NODE = 1,
	MT_NODE_LIST,
	
	MT_INT,
	MT_CHAR,
	MT_STR,
	MT_SIZE,
	MT_BOOL,
	
	MT_NONE
} member_type_t;

typedef enum {
	P_INPUT,
	P_PARSER,
	P_NAMESPACE,
	P_TYPE,
	P_COMPILER,
	P_LINKER
} pass_t;

typedef struct {
	member_type_t type;
	uint32_t offset;
	char* name;
	pass_t pass;
} member_spec_t, *member_spec_p;

typedef struct {
	char*         name;
	uint32_t      components;
	member_spec_p members;
} node_spec_t, *node_spec_p;


//
// List of module types
//

#define BEGIN(nn, NN, c)           NT_##NN,
#define MEMBER(nn, mn, ct, mt, p)  
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

typedef enum {
	NC_NAME    = (1 << 0),
	NC_NS      = (1 << 1),
	NC_VALUE   = (1 << 2),
	NC_STORAGE = (1 << 3),
	NC_EXEC    = (1 << 4),
	NC_BUILDIN = (1 << 5)
} node_component_t;

typedef enum {
	LEFT_TO_RIGHT,
	RIGHT_TO_LEFT
} op_assoc_t;

typedef enum {
	SF_READ  = (1 << 0),
	SF_WRITE = (1 << 1)
} storage_flags_t;

// TODO: update with proper type once we got the compile functions in again
typedef int (*compile_func_t)(node_p node, int ctx, int out);


#define BEGIN(nn, NN, c)           struct {
#define MEMBER(nn, mn, ct, mt, p)  	ct mn;
#define END(nn)                    } nn;

struct node_s {
	node_type_t type;
	node_spec_p spec;
	node_p parent;
	
	token_list_t tokens;
	
	union {
		#include "ast_spec.h"
	};
	
	// name component: node represents something that can be refered to by name
	str_t     name;
	
	// namespace component: new things can be defined in the node
	node_ns_t ns;
	
	// value component: node represents an interim result
	struct {
		type_p type;
	} value;
	
	// storage component: lvalues, node represents a memory block
	struct {
		size_t frame_displ;
		storage_flags_t flags;
	} storage;
	
	// exec component: node represents compilable and runable code
	struct {
		bool   compiled;
		size_t as_offset;
		size_t stack_frame_size;
		list_t(node_addr_slot_t) addr_slots;
		list_t(asm_slot_t)       return_jump_slots;
		
		// TODO: one ASM buffer for compile time execution, one for storage into a binary
		bool linked;
	} exec;
	
	// buildin component: node represents functionality the compiler itself provides
	struct {
		compile_func_t compile_func;
		void*          private;
	} buildin;
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

void node_list_replace_n1(node_list_p list, size_t start_idx, size_t hole_len, node_p replacement_node);
bool node_list_contains_node(node_list_p list, node_p node);

void node_first_token(node_p node, token_p token);
void node_last_token(node_p node, token_p token);

void node_print(node_p node, pass_t pass_min, pass_t pass_max, FILE* output);
void node_print_inline(node_p node, pass_t pass_min, pass_t pass_max, FILE* output);
void node_error(FILE* output, node_p node, const char* message);



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
void     ast_replace_node(node_p node, ast_it_t it, node_p new_child);


//
// Passes
//

void   add_buildin_ops_to_module(node_p module);
node_p pass_resolve_uops(node_p node);
void   fill_namespaces(node_p node, node_ns_p current_ns);