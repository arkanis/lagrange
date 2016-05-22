#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>


typedef struct node_s node_t, *node_p;


//
// Simple variable length buffer
//

#define buf_t(content_type_t) struct { size_t len; content_type_t* ptr; }
#define buf_append(buf, value) do {                                    \
	(buf)->len++;                                                       \
	(buf)->ptr = realloc((buf)->ptr, (buf)->len * sizeof((buf)->ptr[0]));  \
	(buf)->ptr[(buf)->len - 1] = (value);                                \
} while(0)

typedef buf_t(node_p) node_list_t, *node_list_p;


//
// Types for node specs
//

typedef enum {
	MT_NODE = 1,
	MT_NODE_LIST,
	
	MT_INT,
	MT_CHAR,
	MT_CHAR_LIST
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
	NT_SYSCALL,
	
	NT_ID,
	NT_INTL,
	NT_STRL,
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
			node_p expr;
		} syscall;
		
		struct {
			buf_t(char) name;
		} id;
		
		struct {
			int64_t value;
		} intl;
		
		struct {
			buf_t(char) value;
		} strl;
		
		struct {
			node_list_t list;
		} uops;
		
		struct {
			char op;
			node_p a, b;
		} op;
	};
};


//
// Node specs
//

__attribute__ ((weak)) node_spec_p node_specs[] = {
	[ NT_SYSCALL ] = &(node_spec_t){
		"syscall", (member_spec_t[]){
			{ MT_NODE, offsetof(node_t, syscall.expr), "expr" },
			{ 0 }
		}
	},
	
	[ NT_ID ] = &(node_spec_t){
		"id", (member_spec_t[]){
			{ MT_CHAR_LIST, offsetof(node_t, id.name), "name" },
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
			{ MT_CHAR_LIST, offsetof(node_t, strl.value), "value" },
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
			{ MT_CHAR, offsetof(node_t, op.op), "op" },
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

// node_set(parent, member, child)
// node_append(parent, list, child)

#define NI_PRE  (1 << 0)
#define NI_POST (1 << 1)
#define NI_LEAF (1 << 2)
typedef node_p (*node_it_func_t)(node_p node, uint32_t level, uint32_t flags);
node_p node_iterate(node_p node, node_it_func_t func);

void node_print(node_p node, FILE* output);
void node_print_inline(node_p node, FILE* output);