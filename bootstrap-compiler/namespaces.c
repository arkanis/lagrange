#include "common.h"


// Put the slim_hash common code here
#define SLIM_HASH_IMPLEMENTATION
#include "slim_hash.h"

// Generate the implementation for the namespace dictionary that directly uses
// str_t keys. So we don't have to convert between C strings and str_t all the
// time.
SH_GEN_DEF(node_ns, str_t, node_p,
	sh_murmur3_32(key.ptr, key.len),  // hash_expr
	((a.len == b.len) && strncmp(a.ptr, b.ptr, a.len) == 0),  // key_cmp_expr
	(key),  // key_put_expr, no need to duplicate the str
	(key)   // key_del_expr, no need to free the str
);


//
// Pass to fill namespaces with links to their defining nodes
//

void fill_namespaces(node_p node, node_ns_p current_ns) {
	node_ns_p ns_for_children = current_ns;
	
	switch(node->type) {
		case NT_MODULE:
			ns_for_children = &node->module.ns;
			break;
		case NT_FUNC:
			node_ns_put(current_ns, node->func.name, node);
			ns_for_children = &node->func.ns;
			break;
		case NT_SCOPE:
			ns_for_children = &node->scope.ns;
			break;
		case NT_IF_STMT:
			for(size_t i = 0; i < node->if_stmt.true_case.len; i++)
				fill_namespaces(node->if_stmt.true_case.ptr[i], &node->if_stmt.true_ns);
			for(size_t i = 0; i < node->if_stmt.false_case.len; i++)
				fill_namespaces(node->if_stmt.false_case.ptr[i], current_ns);
			// We already iterated over all children so return right away
			return;
		case NT_WHILE_STMT:
			ns_for_children = &node->while_stmt.ns;
		
		case NT_ARG:
			// Don't add unnamed args to the namespace
			if (node->arg.name.len > 0)
				node_ns_put(current_ns, node->arg.name, node);
			break;
		case NT_BINDING:
			node_ns_put(current_ns, node->binding.name, node);
			break;
		
		default:
			for(member_spec_p member = node->spec->members; member->type != 0; member++) {
				if (member->type == MT_NS) {
					fprintf(stderr, "fill_namespaces(): found unhandled node with a namespace!\n");
					abort();
				}
			}
	}
	
	for(ast_it_t it = ast_start(node); it.node != NULL; it = ast_next(node, it))
		fill_namespaces(it.node, ns_for_children);
}



//
// Lookup functions for later passes that use the filled namespaces
//

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
			case NT_IF_STMT:
				if ( node_list_contains_node(&current_node->if_stmt.true_case, child_node) ) {
					// Only look into the true_ns if we came from the true_case list
					// Otherwise we would find bindings from the true_case even when
					// we're in the false_case.
					value = node_ns_get_ptr(&current_node->if_stmt.true_ns, name);
				}
				break;
			case NT_WHILE_STMT:
				value = node_ns_get_ptr(&current_node->while_stmt.ns, name);
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
	
	// For now print an error. Later on we can add a flags argument and callers
	// can request if they want NULL or an error.
	fprintf(stderr, "ns_lookup(): unknown symbol: %.*s\n", name.len, name.ptr);
	abort();
	return NULL;
}
