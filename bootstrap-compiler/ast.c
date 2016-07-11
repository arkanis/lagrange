#include "common.h"

//
// Node specs
//

#define BEGIN(nn, NN)           [ NT_##NN ] = &(node_spec_t){  \
                                    #nn, (member_spec_t[]){
#define MEMBER(nn, mn, ct, mt)  	    { mt, offsetof(node_t, nn.mn), #mn },
#define END(nn)                         { 0 }  \
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



//
// Printing functions
//

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
				if (value != NULL)
					node_print_recursive(value, output, level+1);
				} break;
			case MT_NODE_LIST: {
				node_list_p list = member_ptr;
				for(size_t i = 0; i < list->len; i++) {
					fprintf(output, "\n%*s%s[%zu]: ", (level+1)*2, "", member->name, i);
					node_print_recursive(list->ptr[i], output, level+1);
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
					node_print_inline(asl->ptr[i].target, output);
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
