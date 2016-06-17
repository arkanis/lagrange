#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "slim_hash.h"
#include "utils.h"
#include "asm.h"


typedef struct node_s node_t, *node_p;


//
// Namespaces
//

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

typedef struct {
	size_t len;
	node_p* ptr;
} node_list_t, *node_list_p;

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
	
	MT_INT,
	MT_CHAR,
	MT_STR,
	MT_SIZE,
	MT_BOOL
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


typedef enum {
	NT_MODULE,
	NT_FUNC,
	NT_ARG,
	NT_SCOPE,
	
	NT_VAR,
	NT_IF,
	NT_WHILE,
	NT_RETURN,
	
	NT_ID,
	NT_INTL,
	NT_STRL,
	NT_UNARY_OP,
	NT_UOPS,
	NT_OP,
	NT_CALL,
	
	NT_TYPE_T
} node_type_t;


//
// Node definitions
//

struct node_s {
	node_type_t type;
	node_spec_p spec;
	node_p parent;
	
	union {
		// TODO: Refactor nodes that share the same aspects into "aspect" structures.
		// Canditates:
		//   namespace: module, func, operator, scope, if_stmt (only true case), lambda
		//   asm (as_offset): func, operator, lambda
		//   scope: func, operator, scope, lambda
		
		struct {
			node_list_t defs;
			node_ns_t ns;
		} module;
		
		struct {
			str_t name;
			node_list_t in, out;
			node_list_t body;
			
			node_ns_t ns;
			
			bool compiled, linked;
			size_t as_offset;
			size_t stack_frame_size;
			list_t(node_addr_slot_t) addr_slots;
			list_t(asm_jump_slot_t)  return_jump_slots;
		} func;
		
		struct {
			str_t name;  // Can be 0, NULL in case the arg is unnamed
			node_p type;
			
			int64_t frame_displ;
		} arg;
		
		struct {
			node_list_t stmts;
			node_ns_t ns;
		} scope;
		
		struct {
			str_t name;
			node_p type;
			node_p value;  // Can be NULL in case of declaration only
			
			int64_t frame_displ;
		} var;
		
		struct {
			node_p cond, true_case, false_case;  // false_case can be NULL if there is no else
			node_ns_t true_ns;
		} if_stmt;
		
		struct {
			node_p cond, body;
		} while_stmt;
		
		struct {
			node_list_t args;
		} return_stmt;
		
		
		struct {
			str_t name;
		} id;
		
		struct {
			int64_t value;
		} intl;
		
		struct {
			str_t value;
		} strl;
		
		struct {
			int64_t type;
			node_p arg;
		} unary_op;
		
		struct {
			node_list_t list;
		} uops;
		
		struct {
			int64_t idx;
			node_p a, b;
		} op;
		
		struct {
			str_t name;
			node_list_t args;
		} call;
		
		
		struct {
			str_t name;
			int64_t bits;
		} type_t;
	};
};


//
// Node specs
//

__attribute__ ((weak)) node_spec_p node_specs[] = {
	[ NT_MODULE ] = &(node_spec_t){
		"module", (member_spec_t[]){
			{ MT_NS,        offsetof(node_t, module.ns),   "ns" },
			{ MT_NODE_LIST, offsetof(node_t, module.defs), "defs" },
			{ 0 }
		}
	},
	
	[ NT_FUNC ] = &(node_spec_t){
		"func", (member_spec_t[]){
			{ MT_STR,       offsetof(node_t, func.name), "name" },
			{ MT_NS,        offsetof(node_t, func.ns),   "ns" },
			{ MT_NODE_LIST, offsetof(node_t, func.in),   "in" },
			{ MT_NODE_LIST, offsetof(node_t, func.out),  "out" },
			{ MT_NODE_LIST, offsetof(node_t, func.body), "body" },
			
			{ MT_BOOL,      offsetof(node_t, func.compiled), "compiled" },
			{ MT_BOOL,      offsetof(node_t, func.linked), "linked" },
			{ MT_SIZE,      offsetof(node_t, func.as_offset), "as_offset" },
			{ MT_SIZE,      offsetof(node_t, func.stack_frame_size), "stack_frame_size" },
			{ MT_ASL,       offsetof(node_t, func.addr_slots), "addr_slots" },
			{ 0 }
		}
	},
	
	[ NT_ARG ] = &(node_spec_t){
		"arg", (member_spec_t[]){
			{ MT_STR,  offsetof(node_t, arg.name), "name" },
			{ MT_NODE, offsetof(node_t, arg.type), "type" },
			//{ MT_STR,  offsetof(node_t, arg.type), "type" },
			{ MT_INT,  offsetof(node_t, arg.frame_displ), "frame_displ" },
			{ 0 }
		}
	},
	
	[ NT_SCOPE ] = &(node_spec_t){
		"scope", (member_spec_t[]){
			{ MT_NS,        offsetof(node_t, scope.ns),    "ns" },
			{ MT_NODE_LIST, offsetof(node_t, scope.stmts), "stmts" },
			{ 0 }
		}
	},
	
	[ NT_VAR ] = &(node_spec_t){
		"var", (member_spec_t[]){
			{ MT_STR,  offsetof(node_t, var.name),  "name" },
			{ MT_NODE, offsetof(node_t, var.value), "value" },
			{ MT_NODE, offsetof(node_t, var.type),  "type" },
			{ MT_INT,  offsetof(node_t, arg.frame_displ), "frame_displ" },
			{ 0 }
		}
	},
	
	[ NT_IF ] = &(node_spec_t){
		"if", (member_spec_t[]){
			{ MT_NS,   offsetof(node_t, if_stmt.true_ns),    "true_ns" },
			{ MT_NODE, offsetof(node_t, if_stmt.cond),       "cond" },
			{ MT_NODE, offsetof(node_t, if_stmt.true_case),  "true_case" },
			{ MT_NODE, offsetof(node_t, if_stmt.false_case), "false_case" },
			{ 0 }
		}
	},
	
	[ NT_WHILE ] = &(node_spec_t){
		"while", (member_spec_t[]){
			{ MT_NODE, offsetof(node_t, while_stmt.cond), "cond" },
			{ MT_NODE, offsetof(node_t, while_stmt.body), "body" },
			{ 0 }
		}
	},
	
	[ NT_RETURN ] = &(node_spec_t){
		"return", (member_spec_t[]){
			{ MT_NODE_LIST, offsetof(node_t, return_stmt.args), "args" },
			{ 0 }
		}
	},
	
	[ NT_ID ] = &(node_spec_t){
		"id", (member_spec_t[]){
			{ MT_STR, offsetof(node_t, id.name), "name" },
			{ 0 }
		}
	},
	
	[ NT_INTL ] = &(node_spec_t){
		"intl", (member_spec_t[]){
			{ MT_INT, offsetof(node_t, intl.value), "value" },
			{ 0 }
		}
	},
	
	[ NT_STRL ] = &(node_spec_t){
		"strl", (member_spec_t[]){
			{ MT_STR, offsetof(node_t, strl.value), "value" },
			{ 0 }
		}
	},
	
	[ NT_UNARY_OP ] = &(node_spec_t){
		"unary_op", (member_spec_t[]){
			{ MT_INT,  offsetof(node_t, unary_op.type), "type" },
			{ MT_NODE, offsetof(node_t, unary_op.arg),  "arg" },
			{ 0 }
		}
	},
	
	[ NT_UOPS ] = &(node_spec_t){
		"uops", (member_spec_t[]){
			{ MT_NODE_LIST, offsetof(node_t, uops.list), "list" },
			{ 0 }
		}
	},
	
	[ NT_OP ] = &(node_spec_t){
		"op", (member_spec_t[]){
			{ MT_INT,  offsetof(node_t, op.idx), "idx" },
			{ MT_NODE, offsetof(node_t, op.a), "a" },
			{ MT_NODE, offsetof(node_t, op.b), "b" },
			{ 0 }
		}
	},
	
	[ NT_CALL ] = &(node_spec_t){
		"call", (member_spec_t[]){
			{ MT_STR,       offsetof(node_t, call.name), "name" },
			{ MT_NODE_LIST, offsetof(node_t, call.args), "args" },
			{ 0 }
		}
	},
	
	[ NT_TYPE_T ] = &(node_spec_t){
		"type_t", (member_spec_t[]){
			{ MT_STR,  offsetof(node_t, type_t.name), "name" },
			{ MT_INT,  offsetof(node_t, type_t.bits), "bits" },
		}
	},
};


//
// Node functions
//

node_p node_alloc(node_type_t type);
node_p node_alloc_set(node_type_t type, node_p parent, node_p* member);
node_p node_alloc_append(node_type_t type, node_p parent, node_list_p list);

void node_set(node_p parent, node_p* member, node_p child);
void node_append(node_p parent, node_list_p list, node_p child);

#define NI_PRE  (1 << 0)
#define NI_POST (1 << 1)
#define NI_LEAF (1 << 2)
typedef node_p (*node_it_func_t)(node_p node, uint32_t level, uint32_t flags, void* private);
node_p node_iterate(node_p node, node_it_func_t func, void* private);

void node_print(node_p node, FILE* output);
void node_print_inline(node_p node, FILE* output);


typedef struct {
	ssize_t member_index;
	ssize_t node_index;
	node_p node;
} ast_it_t;

ast_it_t ast_start(node_p node);
ast_it_t ast_next(node_p node, ast_it_t it);

//for(ast_it_t it = ast_start(node); it != NULL; it = ast_next(node, it))
//	foo(it.node);