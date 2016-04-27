#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

/*

# idea sketchpad

node_alloc(name)
node_append(func->defs, child)

str_node_t, str_node_p
str_node_spec
str_node_new("foo bar")

*/

#define buf_t(content_type_t) struct { size_t len; content_type_t* ptr; }
#define buf_append(buf, value) do {                                    \
	(buf).len++;                                                       \
	(buf).ptr = realloc((buf).ptr, (buf).len * sizeof((buf).ptr[0]));  \
	(buf).ptr[(buf).len - 1] = (value);                                \
} while(0)

typedef struct node_s node_t, *node_p;

typedef enum {
	MT_NODE,
	MT_LIST
} member_type_t;

typedef struct {
	member_type_t type;
	uint32_t offset;
	char* name;
} member_spec_t, *member_spec_p;

typedef void (*node_print_func_t)(node_p node, FILE* stream);

typedef struct {
	char* name;
	buf_t(member_spec_t) members;
	// More stuff like function pointers to dump, print, etc. the node
	node_print_func_t print;
} node_spec_t, *node_spec_p;


#define node_alloc(name, parent_node)  ({        \
	name##_p node = calloc(1, sizeof(name##_t)); \
	node->node.spec = name##_spec;               \
	node->node.parent = (node_p)(parent_node);   \
	node;                                        \
})

void node_print(node_p node, FILE* output, int level);



typedef struct module_s    module_t,   *module_p;
typedef struct var_s       var_t,      *var_p;
typedef struct func_s      func_t,     *func_p;
typedef struct ret_stmt_s  ret_stmt_t, *ret_stmt_p;

typedef struct sym_s       sym_t,      *sym_p;
typedef struct int_s       int_t,      *int_p;
typedef struct str_s       str_t,      *str_p;
typedef struct uops_s      uops_t,     *uops_p;

struct node_s {
	node_spec_p spec;
	node_p parent;
};

struct module_s {
	node_t node;
	buf_t(node_p) defs;
	// ADD: module name, symbol table
};

struct var_s {
	node_t node;
	node_p type;
	char*  name;
	node_p value;
};


struct ret_stmt_s {
	node_t node;
	node_p expr;
};

__attribute__((weak)) node_spec_p ret_stmt_spec = &(node_spec_t){
	"ret",
	{ 1,
		(member_spec_t[]){
			{ MT_NODE, offsetof(ret_stmt_t, expr), "expr" }
		}
	},
	NULL
};


struct sym_s {
	node_t node;
	char* name;
	size_t size;
};

static void sym_print(node_p node, FILE* out) {
	sym_p sym_node = (sym_p)node;
	fprintf(out, "%.*s\n", (int)sym_node->size, sym_node->name);
}

__attribute__((weak)) node_spec_p sym_spec = &(node_spec_t){
	"sym",
	{ 0, NULL },
	sym_print
};


struct int_s {
	node_t node;
	int64_t value;
};

static void int_print(node_p node, FILE* out) {
	int_p int_node = (int_p)node;
	fprintf(out, "%ld\n", int_node->value);
}

__attribute__((weak)) node_spec_p int_spec = &(node_spec_t){
	"int",
	{ 0, NULL },
	int_print
};


struct str_s {
	node_t node;
	char* value;
	size_t size;
};

static void str_print(node_p node, FILE* out) {
	str_p str = (str_p)node;
	fprintf(out, "\"%.*s\"\n", (int)str->size, str->value);
}

__attribute__((weak)) node_spec_p str_spec = &(node_spec_t){
	"str",
	{ 0, NULL },
	str_print
};


struct uops_s {
	node_t node;
	buf_t(node_p) list;
};

static void uops_print(node_p node, FILE* out) {
	fprintf(out, "\n");
}

__attribute__((weak)) node_spec_p uops_spec = &(node_spec_t){
	"uops",
	{ 1,
		(member_spec_t[]){
			{ MT_LIST, offsetof(uops_t, list), "list" }
		}
	},
	uops_print
};