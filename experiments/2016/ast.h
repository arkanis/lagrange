#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "utils.h"


typedef struct node_s node_t, *node_p;


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
	
	MT_INT,
	MT_CHAR,
	MT_STR
} member_type_t;

typedef struct {
	member_type_t type;
	uint32_t offset;
	char* name;
} member_spec_t, *member_spec_p;

typedef struct {
	char* name;
	member_spec_p members;
} node_spec_t, *node_spec_p;


typedef enum {
	NT_MODULE,
	NT_SCOPE,
	
	NT_SYSCALL,
	NT_VAR,
	NT_IF,
	
	NT_ID,
	NT_INTL,
	NT_STRL,
	NT_UNARY_OP,
	NT_UOPS,
	NT_OP
} node_type_t;


//
// Node definitions
//

struct node_s {
	node_type_t type;
	node_spec_p spec;
	node_p parent;
	
	union {
		struct {
			node_list_t stmts;
		} module;
		
		struct {
			node_list_t stmts;
		} scope;
		
		struct {
			node_list_t args;
		} syscall;
		
		struct {
			str_t name;
			node_p value;  // Can be NULL in case of declaration only
		} var;
		
		struct {
			node_p cond, true_case, false_case;  // false_case can be NULL if there is no else
		} if_stmt;
		
		
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
	};
};


//
// Node specs
//

__attribute__ ((weak)) node_spec_p node_specs[] = {
	[ NT_MODULE ] = &(node_spec_t){
		"module", (member_spec_t[]){
			{ MT_NODE_LIST, offsetof(node_t, module.stmts), "stmts" },
			{ 0 }
		}
	},
	
	[ NT_SCOPE ] = &(node_spec_t){
		"scope", (member_spec_t[]){
			{ MT_NODE_LIST, offsetof(node_t, scope.stmts), "stmts" },
			{ 0 }
		}
	},
	
	[ NT_SYSCALL ] = &(node_spec_t){
		"syscall", (member_spec_t[]){
			{ MT_NODE_LIST, offsetof(node_t, syscall.args), "args" },
			{ 0 }
		}
	},
	
	[ NT_VAR ] = &(node_spec_t){
		"var", (member_spec_t[]){
			{ MT_STR, offsetof(node_t, var.name), "name" },
			{ MT_NODE, offsetof(node_t, var.value), "value" },
			{ 0 }
		}
	},
	
	[ NT_IF ] = &(node_spec_t){
		"if", (member_spec_t[]){
			{ MT_NODE, offsetof(node_t, if_stmt.cond),       "cond" },
			{ MT_NODE, offsetof(node_t, if_stmt.true_case),  "true_case" },
			{ MT_NODE, offsetof(node_t, if_stmt.false_case), "false_case" },
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
	}
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
typedef node_p (*node_it_func_t)(node_p node, uint32_t level, uint32_t flags);
node_p node_iterate(node_p node, node_it_func_t func);

void node_print(node_p node, FILE* output);
void node_print_inline(node_p node, FILE* output);