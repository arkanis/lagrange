//
// Definitions
//

BEGIN(module, MODULE)
	MEMBER(module, defs, node_list_t, MT_NODE_LIST)
	
	MEMBER(module, ns,   node_ns_t,   MT_NS)
END(module)

BEGIN(func, FUNC)
	MEMBER(func, name, str_t,       MT_STR)
	MEMBER(func, in,   node_list_t, MT_NODE_LIST)
	MEMBER(func, out,  node_list_t, MT_NODE_LIST)
	MEMBER(func, body, node_list_t, MT_NODE_LIST)
	
	MEMBER(func, ns,   node_ns_t,   MT_NS)
	
	MEMBER(func, compiled,          bool,                     MT_BOOL)
	MEMBER(func, linked,            bool,                     MT_BOOL)
	MEMBER(func, as_offset,         size_t,                   MT_SIZE)
	MEMBER(func, stack_frame_size,  size_t,                   MT_SIZE)
	MEMBER(func, addr_slots,        list_t(node_addr_slot_t), MT_ASL )
	MEMBER(func, return_jump_slots, list_t(asm_slot_t),       MT_NONE)
END(func)

BEGIN(arg, ARG)
	// Can be 0, NULL in case the arg is unnamed
	MEMBER(arg, name,        str_t,   MT_STR)
	MEMBER(arg, type_expr,   node_p,  MT_NODE)
	
	MEMBER(arg, type,        type_p,  MT_TYPE)
	MEMBER(arg, frame_displ, int64_t, MT_INT)
END(arg)


//
// Satements
//

BEGIN(scope, SCOPE)
	MEMBER(scope, stmts, node_list_t, MT_NODE_LIST)
	
	MEMBER(scope, ns,    node_ns_t,   MT_NS)
END(scope)

BEGIN(var, VAR)
	MEMBER(var, name, str_t, MT_STR)
	MEMBER(var, type_expr,   type_p,   MT_TYPE)
	MEMBER(var, value, node_p, MT_NODE)
	
	MEMBER(var, type, type_p, MT_TYPE)
	MEMBER(var, frame_displ, int64_t, MT_INT)
END(var)

BEGIN(if_stmt, IF_STMT)
	MEMBER(if_stmt, cond,       node_p,    MT_NODE)
	MEMBER(if_stmt, true_case,  node_p,    MT_NODE)
	MEMBER(if_stmt, false_case, node_p,    MT_NODE)
	
	MEMBER(if_stmt, true_ns,    node_ns_t, MT_NS)
END(if_stmt)

BEGIN(while_stmt, WHILE_STMT)
	MEMBER(while_stmt, cond, node_p, MT_NODE)
	MEMBER(while_stmt, body, node_p, MT_NODE)
END(while_stmt)

BEGIN(return_stmt, RETURN_STMT)
	MEMBER(return_stmt, args, node_list_t, MT_NODE_LIST)
END(return_stmt)


//
// Literals
//

BEGIN(id, ID)
	MEMBER(id, name, str_t, MT_STR)
	
	MEMBER(id, type, type_p, MT_TYPE)
END(id)

BEGIN(intl, INTL)
	MEMBER(intl, value, int64_t, MT_INT)
	
	MEMBER(intl, type, type_p, MT_TYPE)
END(intl)

BEGIN(strl, STRL)
	MEMBER(strl, value, str_t, MT_STR)
	
	MEMBER(strl, type, type_p, MT_TYPE)
END(strl)


//
// Expressions
//

BEGIN(unary_op, UNARY_OP)
	MEMBER(unary_op, index, size_t, MT_SIZE)
	MEMBER(unary_op, arg,   node_p, MT_NODE)
	
	MEMBER(unary_op, type,  type_p, MT_TYPE)
END(unary_op)

BEGIN(uops, UOPS)
	MEMBER(uops, list, node_list_t, MT_NODE_LIST)
	
	// has no type member since it's resolved to op nodes before the type pass
END(uops)

BEGIN(op, OP)
	MEMBER(op, index, size_t, MT_SIZE)
	MEMBER(op, a,     node_p, MT_NODE)
	MEMBER(op, b,     node_p, MT_NODE)
	
	MEMBER(op, type,  type_p, MT_TYPE)
END(op)

BEGIN(call, CALL)
	MEMBER(call, name, str_t,       MT_STR)
	MEMBER(call, args, node_list_t, MT_NODE_LIST)
	
	MEMBER(call, type, type_p,      MT_TYPE)
END(call)