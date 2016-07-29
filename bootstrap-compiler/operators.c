#include "common.h"

// Based on http://en.cppreference.com/w/c/language/operator_precedence
// Worth a look because of bitwise and comparison ops: http://wiki.dlang.org/Operator_precedence
// CAUTION: Operators with the same precedence need the same associativity!
// Otherwise it probably gets complicates... not sure.
struct { char* name; int precedence; op_assoc_t assoc; } operators[] = {
	[T_MUL]    = { "mul",  80, LEFT_TO_RIGHT },
	[T_DIV]    = { "div",  80, LEFT_TO_RIGHT },
	[T_MOD]    = { "mod",  80, LEFT_TO_RIGHT },
	
	[T_ADD]    = { "add",  70, LEFT_TO_RIGHT },
	[T_SUB]    = { "sub",  70, LEFT_TO_RIGHT },
	
	[T_LT]     = { "lt",   50, LEFT_TO_RIGHT },
	[T_LE]     = { "le",   50, LEFT_TO_RIGHT },
	[T_GT]     = { "gt",   50, LEFT_TO_RIGHT },
	[T_GE]     = { "ge",   50, LEFT_TO_RIGHT },
	
	[T_EQ]     = { "eq",   40, LEFT_TO_RIGHT },
	[T_NEQ]    = { "neq",  40, LEFT_TO_RIGHT },
	
	[T_ASSIGN] = { "assign", 0, RIGHT_TO_LEFT },
};


/**
 * For each uops node: We find the strongest binding operator and replace it and
 * the nodes it binds to with an op node. This is repeated until no operators
 * are left to replace. The uops node should just have one op child at the end.
 */
node_p pass_resolve_uops(module_p module, node_p node) {
	// Frist resolve all uops in the child nodes. This needs less recursive
	// calls than doing it afterwards.
	for(ast_it_t it = ast_start(node); it.node != NULL; it = ast_next(node, it)) {
		node_p new_child = pass_resolve_uops(module, it.node);
		if (new_child != it.node)
			ast_replace_node(node, it, new_child);
	}
	
	// Leave non uops nodes untouched
	if (node->type != NT_UOPS)
		return node;
	
	node_list_p list = &node->uops.list;
	
	while (list->len > 1) {
		// Find strongest operator (operator with highest precedence)
		int strongest_op_idx = -1;
		int strongest_op_prec = 0;
		int strongest_op_node_idx = -1;
		for(int node_idx = 1; node_idx < (int)list->len; node_idx += 2) {
			node_p op_slot = list->ptr[node_idx];
			if (op_slot->type != NT_ID) {
				node_error(stderr, op_slot, module, "pass_resolve_uops(): got non NT_ID in uops op slot!\n");
				abort();
			}
			
			// Find operator of the current op_slot node based on the IDs name
			ssize_t op_idx = -1;
			for(size_t i = 0; i < sizeof(operators) / sizeof(operators[0]); i++) {
				// Ignore holes in the operators array since we left out some
				// operator IDs.
				if (operators[i].name == NULL)
					continue;
				
				if ( strncmp(operators[i].name, op_slot->id.name.ptr, op_slot->id.name.len) == 0 && (int)strlen(operators[i].name) == op_slot->id.name.len ) {
					op_idx = i;
					break;
				}
			}
			
			if (op_idx == -1) {
				node_error(stderr, op_slot, module, "pass_resolve_uops(): got unimplemented operator!\n");
				abort();
			}
			
			//printf("pass_resolve_uops(): got op %.*s (idx %zd) at node idx %d\n",
			//	op_slot->id.name.len, op_slot->id.name.ptr, op_idx, node_idx
			//);
			
			// When several ops have the highest (same) precedence:
			// For LEFT_TO_RIGHT operators we pick the first op
			// For RIGHT_TO_LEFT operators we pick the last op
			if (
				(operators[op_idx].assoc == LEFT_TO_RIGHT && operators[op_idx].precedence >  strongest_op_prec) ||
				(operators[op_idx].assoc == RIGHT_TO_LEFT && operators[op_idx].precedence >= strongest_op_prec)
			) {
				strongest_op_prec = operators[op_idx].precedence;
				strongest_op_idx = op_idx;
				strongest_op_node_idx = node_idx;
			}
		}
		
		if (strongest_op_idx == -1) {
			node_error(stderr, node, module, "pass_resolve_uops(): no operators found in uops node!\n");
			abort();
		}
		
		//printf("pass_resolve_uops(): op %s got highest precedence: %d\n",
		//	operators[strongest_op_idx].name, strongest_op_prec
		//);
		
		
		// Take the nodes left and right from the op_slot node and create a new op
		// node out of them. Then replace these nodes in the list with the new op
		// node.
		node_p op_node = node_alloc(NT_OP);
		op_node->parent = node;
		
		op_node->op.name = list->ptr[strongest_op_node_idx]->id.name;
		node_p untranslated_op = node_alloc_set(NT_ID, op_node, &op_node->op.op);
		untranslated_op->id.name = list->ptr[strongest_op_node_idx]->tokens.ptr->source;
		node_first_token(untranslated_op, list->ptr[strongest_op_node_idx]->tokens.ptr);
		
		node_set(op_node, &op_node->op.a, list->ptr[strongest_op_node_idx - 1]);
		node_set(op_node, &op_node->op.b, list->ptr[strongest_op_node_idx + 1]);
		
		node_first_token(op_node, op_node->op.a->tokens.ptr);
		node_last_token(op_node, op_node->op.b->tokens.ptr);
		
		node_list_replace_n1(list, strongest_op_node_idx - 1, 3, op_node);
		
		//node_print(node, P_PARSER, stderr);
	}
	
	// By now the uops node only contains one op node child. Return that so the
	// recursive iteration code above replaces this uops node with the returned
	// op node.
	return node->uops.list.ptr[0];
}