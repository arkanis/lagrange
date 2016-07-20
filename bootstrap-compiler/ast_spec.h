//
// Definitions
//

BEGIN(module, MODULE)
	MEMBER(module, defs, node_list_t, MT_NODE_LIST, P_PARSER)
	
	MEMBER(module, ns,   node_ns_t,   MT_NS,        P_NAMESPACE)
END(module)

BEGIN(func, FUNC)
	MEMBER(func, name, str_t,       MT_STR,       P_PARSER)
	MEMBER(func, in,   node_list_t, MT_NODE_LIST, P_PARSER)
	MEMBER(func, out,  node_list_t, MT_NODE_LIST, P_PARSER)
	MEMBER(func, body, node_list_t, MT_NODE_LIST, P_PARSER)
	
	MEMBER(func, ns,   node_ns_t,   MT_NS,        P_NAMESPACE)
	
	MEMBER(func, compiled,          bool,                     MT_BOOL, P_COMPILER)
	MEMBER(func, as_offset,         size_t,                   MT_SIZE, P_COMPILER)
	MEMBER(func, stack_frame_size,  size_t,                   MT_SIZE, P_COMPILER)
	MEMBER(func, addr_slots,        list_t(node_addr_slot_t), MT_ASL,  P_COMPILER)
	MEMBER(func, return_jump_slots, list_t(asm_slot_t),       MT_NONE, P_COMPILER)
	
	MEMBER(func, linked,            bool,                     MT_BOOL, P_LINKER)
END(func)

BEGIN(arg, ARG)
	// Can be 0, NULL in case the arg is unnamed
	MEMBER(arg, name,        str_t,   MT_STR,  P_PARSER)
	MEMBER(arg, type_expr,   node_p,  MT_NODE, P_PARSER)
	
	MEMBER(arg, type,        type_p,  MT_TYPE, P_TYPE)
	MEMBER(arg, frame_displ, int64_t, MT_INT,  P_COMPILER)
END(arg)


//
// Satements
//

BEGIN(scope, SCOPE)
	MEMBER(scope, stmts, node_list_t, MT_NODE_LIST, P_PARSER)
	
	MEMBER(scope, ns,    node_ns_t,   MT_NS,        P_NAMESPACE)
END(scope)

BEGIN(var, VAR)
	MEMBER(var, type_expr, node_p,      MT_NODE,      P_PARSER)
	MEMBER(var, bindings,  node_list_t, MT_NODE_LIST, P_PARSER)
END(var)

BEGIN(binding, BINDING)
	MEMBER(binding, name,        str_t,   MT_STR,  P_PARSER)
	MEMBER(binding, value,       node_p,  MT_NODE, P_PARSER)
	
	MEMBER(binding, type,        type_p,  MT_TYPE, P_TYPE)
	MEMBER(binding, frame_displ, int64_t, MT_INT,  P_COMPILER)
END(binding)

BEGIN(if_stmt, IF_STMT)
	MEMBER(if_stmt, cond,       node_p,      MT_NODE,      P_PARSER)
	MEMBER(if_stmt, true_case,  node_list_t, MT_NODE_LIST, P_PARSER)
	MEMBER(if_stmt, false_case, node_list_t, MT_NODE_LIST, P_PARSER)
	
	MEMBER(if_stmt, true_ns,    node_ns_t,   MT_NS,        P_NAMESPACE)
END(if_stmt)

BEGIN(while_stmt, WHILE_STMT)
	MEMBER(while_stmt, cond, node_p,      MT_NODE,      P_PARSER)
	MEMBER(while_stmt, body, node_list_t, MT_NODE_LIST, P_PARSER)
	
	MEMBER(while_stmt, ns,   node_ns_t,   MT_NS,        P_NAMESPACE)
END(while_stmt)

BEGIN(return_stmt, RETURN_STMT)
	MEMBER(return_stmt, args, node_list_t, MT_NODE_LIST, P_PARSER)
END(return_stmt)


//
// Literals
//

BEGIN(id, ID)
	MEMBER(id, name, str_t,  MT_STR,  P_PARSER)
	
	MEMBER(id, type, type_p, MT_TYPE, P_TYPE)
END(id)

BEGIN(intl, INTL)
	MEMBER(intl, value, int64_t, MT_INT,  P_PARSER)
	
	MEMBER(intl, type,  type_p,  MT_TYPE, P_TYPE)
END(intl)

BEGIN(strl, STRL)
	MEMBER(strl, value, str_t,  MT_STR,  P_PARSER)
	
	MEMBER(strl, type,  type_p, MT_TYPE, P_TYPE)
END(strl)


//
// Expressions
//

BEGIN(unary_op, UNARY_OP)
	MEMBER(unary_op, name, str_t,  MT_STR,  P_PARSER)
	MEMBER(unary_op, op,   node_p, MT_NODE, P_PARSER)
	MEMBER(unary_op, arg,  node_p, MT_NODE, P_PARSER)
	
	MEMBER(unary_op, type, type_p, MT_TYPE, P_TYPE)
END(unary_op)

BEGIN(uops, UOPS)
	MEMBER(uops, list, node_list_t, MT_NODE_LIST, P_PARSER)
	// uops nodes are resolved directly after the parser pass so no member for
	// further passes.
END(uops)

BEGIN(op, OP)
	MEMBER(op, name, str_t,  MT_STR,  P_PARSER)
	MEMBER(op, op,   node_p, MT_NODE, P_PARSER)
	MEMBER(op, a,    node_p, MT_NODE, P_PARSER)
	MEMBER(op, b,    node_p, MT_NODE, P_PARSER)
	
	MEMBER(op, type, type_p, MT_TYPE, P_TYPE)
END(op)

BEGIN(member, MEMBER)
	MEMBER(member, aggregate, node_p, MT_NODE, P_PARSER)
	MEMBER(member, member,    str_t,  MT_STR,  P_PARSER)
	
	MEMBER(member, type,      type_p, MT_TYPE, P_TYPE)
END(member)

BEGIN(call, CALL)
	MEMBER(call, target_expr, node_p,      MT_NODE,      P_PARSER)
	MEMBER(call, args,        node_list_t, MT_NODE_LIST, P_PARSER)
	
	MEMBER(call, type,        type_p,      MT_TYPE,      P_TYPE)
END(call)

BEGIN(index, INDEX)
	MEMBER(index, target_expr, node_p,      MT_NODE,      P_PARSER)
	MEMBER(index, args,        node_list_t, MT_NODE_LIST, P_PARSER)
	
	MEMBER(index, type,        type_p,      MT_TYPE,      P_TYPE)
END(index)