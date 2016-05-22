#include <stdbool.h>
#include <stdlib.h>
#include "ast.h"


//
// Allocation functions
//

node_p node_alloc(node_type_t type) {
	node_p node = calloc(1, sizeof(node_t));
	
	node->type = type;
	node->spec = node_specs[type];
	node->parent = NULL;
	
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
	buf_append(list, node);
	
	return node;
}


//
// Other functions
//

static node_p node_iterate_recursive(node_p node, uint32_t level, node_it_func_t func) {
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
		return func(node, level, NI_LEAF);
	}
	
	// Node has children
	node = func(node, level, NI_PRE);
	
	for(member_spec_p member = node->spec->members; member->type != 0; member++) {
		void* member_ptr = (uint8_t*)node + member->offset;
		if (member->type == MT_NODE) {
			node_p* child_node = member_ptr;
			*child_node = node_iterate_recursive(*child_node, level + 1, func);
		} else if (member->type == MT_NODE_LIST) {
			node_list_p list = member_ptr;
			for(size_t i = 0; i < list->len; i++)
				list->ptr[i] = node_iterate_recursive(list->ptr[i], level + 1, func);
		}
	}
	
	return func(node, level, NI_POST);
}

node_p node_iterate(node_p node, node_it_func_t func) {
	return node_iterate_recursive(node, 0, func);
}



static void node_print_recursive(node_p node, FILE* output, int level) {
	fprintf(output, "%s: ", node->spec->name);
	
	for(member_spec_p member = node->spec->members; member->type != 0; member++) {
		// Print the first non-node member inline (no line break, no indention,
		// don't print the member name). MT_NODE_LIST also prints it's member
		// name by itself (with an index).
		bool print_without_label = (member == node->spec->members && member->type != MT_NODE) || member->type == MT_NODE_LIST;
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
			
			case MT_INT: {
				int64_t* value = member_ptr;
				fprintf(output, "%ld", *value);
				} break;
			case MT_CHAR: {
				char* value = member_ptr;
				fprintf(output, "%c", *value);
				} break;
			case MT_CHAR_LIST: {
				buf_t(char)* value = member_ptr;
				fprintf(output, "\"%.*s\"", (int)value->len, value->ptr);
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
			
			case MT_INT: {
				int64_t* value = member_ptr;
				fprintf(output, "%ld", *value);
				} break;
			case MT_CHAR: {
				char* value = member_ptr;
				fprintf(output, "%c", *value);
				} break;
			case MT_CHAR_LIST: {
				buf_t(char)* value = member_ptr;
				fprintf(output, "\"%.*s\"", (int)value->len, value->ptr);
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