#include "common.h"

//
// Node specs
//

#define BEGIN(nn, NN)              [ NT_##NN ] = &(node_spec_t){  \
                                       #nn, (member_spec_t[]){
#define MEMBER(nn, mn, ct, mt, p)          { mt, offsetof(node_t, nn.mn), #mn, p },
#define END(nn)                            { 0 }  \
                                       }  \
                                   },

node_spec_p* node_specs = (node_spec_p[]){
	#include "ast_spec.h"
};

#undef BEGIN
#undef MEMBER
#undef END



//
// Allocation functions
//

node_p node_alloc(node_type_t type) {
	node_p node = calloc(1, sizeof(node_t));
	
	node->type = type;
	node->spec = node_specs[type];
	node->parent = NULL;
	
	// TODO: reenable once we got namespaces to work again
	/*
	for(member_spec_p member = node->spec->members; member->type != 0; member++) {
		void* member_ptr = (uint8_t*)node + member->offset;
		if (member->type == MT_NS) {
			node_ns_p namespace = member_ptr;
			node_ns_new(namespace);
		}
	}
	*/
	
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
	list_append(list, node);
	
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
	list_append(list, child);
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
// Functions to assign tokens to nodes
//

void node_first_token(node_p node, token_p token) {
	if (node->tokens.ptr) {
		node->tokens.len -= token - node->tokens.ptr;
		node->tokens.ptr = token;
	} else {
		node->tokens.ptr = token;
		node->tokens.len = 1;
	}
}

void node_last_token(node_p node, token_p token) {
	if (token < node->tokens.ptr) {
		fprintf(stderr, "node_last_token(): Tried to assign a last token that's before the first token of the node!\n");
		abort();
	} else if (!node->tokens.ptr) {
		fprintf(stderr, "node_last_token(): Tried to assign a last token before assigning a first token!\n");
		abort();
	}
	
	node->tokens.len = token - node->tokens.ptr + 1;
}



//
// Printing functions
//

static void node_print_recursive(node_p node, pass_t pass, FILE* output, int level) {
	fprintf(output, "%s: ", node->spec->name);
	
	for(member_spec_p member = node->spec->members; member->type != 0; member++) {
		// Skip members from later passes. So we only show members up to and
		// including the requested pass.
		if (member->pass > pass)
			continue;
		
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
				if (value != NULL)
					node_print_recursive(value, pass, output, level+1);
				} break;
			case MT_NODE_LIST: {
				node_list_p list = member_ptr;
				for(size_t i = 0; i < list->len; i++) {
					fprintf(output, "\n%*s%s[%zu]: ", (level+1)*2, "", member->name, i);
					node_print_recursive(list->ptr[i], pass, output, level+1);
				}
				} break;
			case MT_NS: {
				/*
				node_ns_p ns = member_ptr;
				for(node_ns_it_p it = node_ns_start(ns); it != NULL; it = node_ns_next(ns, it)) {
					fprintf(output, "\"%.*s\" ", it->key.len, it->key.ptr);
				}
				*/
				} break;
			case MT_ASL: {
				list_t(node_addr_slot_t)* asl = member_ptr;
				for(size_t i = 0; i < asl->len; i++) {
					fprintf(output, "\n%*soffset %zu → ", (level+2)*2, "", asl->ptr[i].offset);
					node_print_inline(asl->ptr[i].target, pass, output);
					fprintf(output, "\n");
				}
				} break;
			case MT_TYPE: {
				/*
				type_p type = *(type_p*)member_ptr;
				if (type == NULL)
					fprintf(output, "NULL");
				else
					fprintf(output, "%.*s (%zu bytes)", type->name.len, type->name.ptr, type->size);
				*/
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
			case MT_NONE:
				fprintf(output, "???");
				break;
		}
	}
}

void node_print(node_p node, pass_t pass, FILE* output) {
	node_print_recursive(node, pass, output, 0);
	fprintf(output, "\n");
}

void node_print_inline(node_p node, pass_t pass, FILE* output) {
	fprintf(output, "%s", node->spec->name);
	
	for(member_spec_p member = node->spec->members; member->type != 0; member++) {
		// Skip members from later passes. So we only show members up to and
		// including the requested pass.
		if (member->pass > pass)
			continue;
		
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
			case MT_TYPE: {
				/*
				type_p type = *(type_p*)member_ptr;
				if (type == NULL)
					fprintf(output, "NULL");
				else
					fprintf(output, "%.*s (%zu bytes)", type->name.len, type->name.ptr, type->size);
				*/
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
			case MT_NONE:
				fprintf(output, "???");
				break;
		}
	}
}


//
// AST iteration functions
//

/*
new iterator
	search for first node or list
	node:
		member_index = idx of first member
		node_index = -1
		node = ptr
	list:
		member_index = idx of first member
		node_index = 0
		node = list.ptr[0]

next iterator:
	if member[member_idx] is node
		new iterator for member_idx+1
	is list
		node_index++
		if node_index < list.len
			node = list.ptr[node_index]
		else
			new iterator for member_idx+1
*/

static ast_it_t ast_it_for_node(node_p node, size_t member_index) {
	for(size_t i = member_index; node->spec->members[i].type != 0; i++) {
		member_spec_p member = &node->spec->members[i];
		void* member_ptr = (uint8_t*)node + member->offset;
		
		if (member->type == MT_NODE) {
			node_p* child_node = member_ptr;
			if (*child_node) {
				return (ast_it_t){
					.member_index = i,
					.node_index = -1,
					.node = *child_node
				};
			}
		} else if (member->type == MT_NODE_LIST) {
			node_list_p list = member_ptr;
			if (list->len > 0) {
				return (ast_it_t){
					.member_index = i,
					.node_index = 0,
					.node = list->ptr[0]
				};
			}
		}
	}
	
	return (ast_it_t){
		.member_index = -1,
		.node_index = -1,
		.node = NULL
	};
}

ast_it_t ast_start(node_p node) {
	return ast_it_for_node(node, 0);
}

ast_it_t ast_next(node_p node, ast_it_t it) {
	if (it.member_index == -1)
		return it;
	
	member_spec_p member = &node->spec->members[it.member_index];
	void* member_ptr = (uint8_t*)node + member->offset;
	if ( member->type == MT_NODE_LIST ) {
		node_list_p list = member_ptr;
		it.node_index++;
		if (it.node_index < (ssize_t)list->len) {
			it.node = list->ptr[it.node_index];
			return it;
		}
	}
	
	return ast_it_for_node(node, it.member_index + 1);
}

void ast_replace_node(node_p node, ast_it_t it, node_p new_child) {
	if (it.member_index == -1 || it.node == NULL) {
		fprintf(stderr, "ast_replace_node(): Tried to replace a node with an iterator that doesn't point to a node!\n");
		abort();
	}
	
	// Kill parent link in old child node and set it in the new child node
	it.node->parent = NULL;
	new_child->parent = node;
	
	member_spec_p member_spec = &node->spec->members[it.member_index];
	void*         member_ptr  = (uint8_t*)node + member_spec->offset;
	
	if ( member_spec->type == MT_NODE ) {
		node_p* child_node = member_ptr;
		(*child_node)->parent = NULL;
		*child_node = new_child;
	} else if ( member_spec->type == MT_NODE_LIST ) {
		node_list_p list = member_ptr;
		list->ptr[it.node_index]->parent = NULL;
		list->ptr[it.node_index] = new_child;
	} else {
		fprintf(stderr, "ast_replace_node(): The iterator doesn't point to a node or node list member!\n");
		abort();
	}
}