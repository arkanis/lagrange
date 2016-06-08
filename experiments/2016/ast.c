// For strdup
#define _GNU_SOURCE

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

//
// Namespaces
//

#define SLIM_HASH_IMPLEMENTATION
#include "slim_hash.h"
SH_GEN_DEF(node_ns, str_t, node_p,
	node_ns_murmur3_32(key.ptr, key.len),  // hash_expr
	((a.len == b.len) && strncmp(a.ptr, b.ptr, a.len) == 0),  // key_cmp_expr
	(key),  // key_put_expr, no need to duplicate the str
	(key)   // key_del_expr, no need to free the str
);

node_p ns_lookup(node_p node, str_t name) {
	node_p current_node = node, child_node = NULL;
	
	while (current_node != NULL) {
		node_p* value = NULL;
		switch(current_node->type) {
			case NT_MODULE:
				value = node_ns_get_ptr(&current_node->module.ns, name);
				break;
			case NT_FUNC:
				value = node_ns_get_ptr(&current_node->func.ns, name);
				break;
			case NT_SCOPE:
				value = node_ns_get_ptr(&current_node->scope.ns, name);
				break;
			case NT_IF:
				if (child_node == current_node->if_stmt.true_case) {
					// Only look into the true_ns if we came from the true_case node
					// Otherwise we would find bindings for the true_case even when
					// we're in the false_case.
					value = node_ns_get_ptr(&current_node->if_stmt.true_ns, name);
				}
				break;
			
			default:
				for(member_spec_p member = current_node->spec->members; member->type != 0; member++) {
					if (member->type == MT_NS) {
						fprintf(stderr, "ns_lookup(): found unhandled node with a namespace!\n");
						abort();
					}
				}
		}
		
		if (value)
			return *value;
		
		child_node = current_node;
		current_node = current_node->parent;
	}
	
	return NULL;
}


//
// Node list
//

void node_list_append(node_list_p list, node_p node) {
	list->len++;
	list->ptr = realloc(list->ptr, list->len * sizeof(list->ptr[0]));
	list->ptr[list->len - 1] = node;
}

void node_list_replace_n1(node_list_p list, size_t start_idx, size_t hole_len, node_p replacement_node) {
	size_t new_len = list->len - hole_len + 1;
	
	// Nodes from 0..start_idx remain unchanged
	// Node at start_idx is replaced by replacement_node
	// Overwrite nodes begining at start_idx+1 with nodes at (start_idx + hole_len)..new_len
	list->ptr[start_idx] = replacement_node;
	for(size_t i = start_idx + 1; i < new_len; i++)
		list->ptr[i] = list->ptr[i + hole_len - 1];
	// Null out free space
	for(size_t i = new_len; i < list->len; i++)
		list->ptr[i] = NULL;
	
	list->len = new_len;
	list->ptr = realloc(list->ptr, list->len * sizeof(list->ptr[0]));
}


//
// Allocation functions
//

node_p node_alloc(node_type_t type) {
	node_p node = calloc(1, sizeof(node_t));
	
	node->type = type;
	node->spec = node_specs[type];
	node->parent = NULL;
	
	for(member_spec_p member = node->spec->members; member->type != 0; member++) {
		void* member_ptr = (uint8_t*)node + member->offset;
		if (member->type == MT_NS) {
			node_ns_p namespace = member_ptr;
			node_ns_new(namespace);
		}
	}
	
	return node;
}

node_p node_alloc_set(node_type_t type, node_p parent, node_p* member) {
	node_p node = node_alloc(type);
	
	node->parent = parent;
	*member = node;
	
	return node;
}

node_p node_alloc_append(node_type_t type, node_p parent, node_list_p list) {
	node_p node = node_alloc(type);
	
	node->parent = parent;
	node_list_append(list, node);
	
	return node;
}



//
// Manipulation functions
//

void node_set(node_p parent, node_p* member, node_p child) {
	if ( (size_t)((uint8_t*)member - (uint8_t*)parent) > sizeof(node_t) ) {
		fprintf(stderr, "node_set(): member isn't part of the parent node!\n");
		abort();
	}
	
	child->parent = parent;
	*member = child;
}

void node_append(node_p parent, node_list_p list, node_p child) {
	if ( (size_t)((uint8_t*)list - (uint8_t*)parent) > sizeof(node_t) ) {
		fprintf(stderr, "node_append(): list isn't part of the parent node!\n");
		abort();
	}
	
	child->parent = parent;
	node_list_append(list, child);
}



//
// Other functions
//

static node_p node_iterate_recursive(node_p node, uint32_t level, node_it_func_t func, void* private) {
	// Check if node has no children
	bool has_children = false;
	for(member_spec_p member = node->spec->members; member->type != 0; member++) {
		void* member_ptr = (uint8_t*)node + member->offset;
		if (member->type == MT_NODE) {
			node_p* child_node = member_ptr;
			if (*child_node != NULL) {
				has_children = true;
				break;
			}
		} else if (member->type == MT_NODE_LIST) {
			node_list_p list = member_ptr;
			if (list->len > 0) {
				has_children = true;
				break;
			}
		}
	}
	
	if (!has_children) {
		// Node has no children
		return func(node, level, NI_LEAF, private);
	}
	
	// Node has children
	node = func(node, level, NI_PRE, private);
	
	for(member_spec_p member = node->spec->members; member->type != 0; member++) {
		void* member_ptr = (uint8_t*)node + member->offset;
		if (member->type == MT_NODE) {
			node_p* child_node = member_ptr;
			*child_node = node_iterate_recursive(*child_node, level + 1, func, private);
		} else if (member->type == MT_NODE_LIST) {
			node_list_p list = member_ptr;
			for(size_t i = 0; i < list->len; i++)
				list->ptr[i] = node_iterate_recursive(list->ptr[i], level + 1, func, private);
		}
	}
	
	return func(node, level, NI_POST, private);
}

node_p node_iterate(node_p node, node_it_func_t func, void* private) {
	return node_iterate_recursive(node, 0, func, private);
}



static void node_print_recursive(node_p node, FILE* output, int level) {
	fprintf(output, "%s: ", node->spec->name);
	
	for(member_spec_p member = node->spec->members; member->type != 0; member++) {
		// Print the first non-node member inline (no line break, no indention,
		// don't print the member name). MT_NODE_LIST also prints it's member
		// name by itself (with an index).
		bool print_without_label = (member == node->spec->members && member->type != MT_NODE && member->type != MT_NS) || member->type == MT_NODE_LIST;
		if ( !print_without_label )
			fprintf(output, "\n%*s%s: ", (level+1)*2, "", member->name);
		
		void* member_ptr = (uint8_t*)node + member->offset;
		switch(member->type) {
			case MT_NODE: {
				node_p value = *(node_p*)member_ptr;
				node_print_recursive(value, output, level+1);
				} break;
			case MT_NODE_LIST: {
				node_list_p list = member_ptr;
				for(size_t i = 0; i < list->len; i++) {
					fprintf(output, "\n%*s%s[%zu]: ", (level+1)*2, "", member->name, i);
					node_print_recursive(list->ptr[i], output, level+1);
				}
				} break;
			case MT_NS:{
				node_ns_p ns = member_ptr;
				for(node_ns_it_p it = node_ns_start(ns); it != NULL; it = node_ns_next(ns, it)) {
					fprintf(output, "\"%.*s\" ", it->key.len, it->key.ptr);
				}
				} break;
			case MT_ASL:{
				list_t(node_addr_slot_t)* asl = member_ptr;
				for(size_t i = 0; i < asl->len; i++) {
					fprintf(output, "\n%*soffset %zu → ", (level+2)*2, "", asl->ptr[i].offset);
					node_print_inline(asl->ptr[i].target, output);
					fprintf(output, "\n");
				}
				} break;
			
			case MT_INT: {
				int64_t* value = member_ptr;
				fprintf(output, "%ld", *value);
				} break;
			case MT_CHAR: {
				char* value = member_ptr;
				fprintf(output, "%c", *value);
				} break;
			case MT_STR: {
				str_p value = member_ptr;
				fprintf(output, "\"%.*s\"", value->len, value->ptr);
				} break;
			case MT_SIZE: {
				size_t* value = member_ptr;
				fprintf(output, "%zu", *value);
				} break;
			case MT_BOOL: {
				bool* value = member_ptr;
				fprintf(output, "%s", *value ? "true" : "false");
				} break;
		}
	}
}

void node_print(node_p node, FILE* output) {
	node_print_recursive(node, output, 0);
	fprintf(output, "\n");
}

void node_print_inline(node_p node, FILE* output) {
	fprintf(output, "%s", node->spec->name);
	
	for(member_spec_p member = node->spec->members; member->type != 0; member++) {
		fprintf(output, " %s: ", member->name);
		
		void* member_ptr = (uint8_t*)node + member->offset;
		switch(member->type) {
			case MT_NODE:
			case MT_NODE_LIST:
				fprintf(output, "...");
				break;
			
			case MT_NS:
				fprintf(output, "ns");
				break;
			case MT_ASL:
				fprintf(output, "asl");
				break;
			case MT_INT: {
				int64_t* value = member_ptr;
				fprintf(output, "%ld", *value);
				} break;
			case MT_CHAR: {
				char* value = member_ptr;
				fprintf(output, "%c", *value);
				} break;
			case MT_STR: {
				str_p value = member_ptr;
				fprintf(output, "\"%.*s\"", value->len, value->ptr);
				} break;
			case MT_SIZE: {
				size_t* value = member_ptr;
				fprintf(output, "%zu", *value);
				} break;
			case MT_BOOL: {
				bool* value = member_ptr;
				fprintf(output, "%s", *value ? "true" : "false");
				} break;
		}
	}
}


/*
node_p demo_pass(node_p node, uint32_t level, uint32_t flags) {
	if (flags & NI_PRE) {
		printf("%*sPRE  %p ", (int)level*2, "", node);
		node_print_inline(node, stdout);
		printf("\n");
	} else if (flags & NI_POST) {
		printf("%*sPOST %p ", (int)level*2, "", node);
		node_print_inline(node, stdout);
		printf("\n");
	} else if (flags & NI_LEAF) {
		printf("%*sLEAF %p ", (int)level*2, "", node);
		node_print_inline(node, stdout);
		printf("\n");
	}
	return node;
}

int main() {
	node_p syscall = node_alloc(NT_SYSCALL);
		node_p op = node_alloc_set(NT_OP, syscall, &syscall->syscall.expr);
			op->op.op = '+';
			node_p a = node_alloc_set(NT_INTL, op, &op->op.a);
				a->intl.value = 17;
			node_p b = node_alloc_set(NT_UOPS, op, &op->op.b);
				node_p b1 = node_alloc_append(NT_INTL, b, &b->uops.list);
					b1->intl.value = 3;
				node_p b2 = node_alloc_append(NT_STRL, b, &b->uops.list);
					b2->strl.value.ptr = "foo";
					b2->strl.value.len = 3;
	
	node_print(syscall, stdout);
	
	node_iterate(syscall, demo_pass);
	
	return 0;
}
*/