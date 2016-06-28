#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "slim_hash.h"
#include "utils.h"
#include "asm.h"


typedef struct node_s node_t, *node_p;
typedef struct type_s type_t, *type_p;


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

#define BEGIN(nn)               NT_##nn,
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

#define BEGIN(nn)               struct {
#define MEMBER(nn, mn, ct, mt)  	ct mn;
#define END(nn)                 } nn;

struct node_s {
	node_type_t type;
	node_spec_p spec;
	node_p parent;
	
	union {
		#include "ast_spec.h"
	};
};

#undef BEGIN
#undef MEMBER
#undef END

// Actual values get defined in ast.c
node_spec_p node_specs[];