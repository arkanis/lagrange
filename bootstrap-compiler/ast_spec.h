#ifndef BEGIN
	#define BEGIN(node_name, node_name_in_capitals, components)
#endif
#ifndef MEMBER
	#define MEMBER(node_name, member_name, c_type, member_type, pass)
#endif
#ifndef END
	#define END(node_name)
#endif


//
// Definitions
//

BEGIN(module, MODULE, NC_NS | NC_NAME)
	MEMBER(module, filename, str_t,        MT_STR,  P_INPUT)
	MEMBER(module, source,   str_t,        MT_NONE, P_INPUT)
	
	MEMBER(module, body, node_list_t, MT_NODE_LIST, P_PARSER)
END(module)

BEGIN(func_def, FUNC_DEF, NC_NS | NC_NAME | NC_EXEC)
	MEMBER(func_def, in,   node_list_t, MT_NODE_LIST, P_PARSER)
	MEMBER(func_def, out,  node_list_t, MT_NODE_LIST, P_PARSER)
	MEMBER(func_def, body, node_list_t, MT_NODE_LIST, P_PARSER)
END(func_def)

BEGIN(func_buildin, FUNC_BUILDIN, NC_NS | NC_NAME | NC_BUILDIN)
END(func_buildin)

BEGIN(op_def, OP_DEF, NC_NS | NC_NAME | NC_EXEC)
	MEMBER(op_def, in,      node_list_t, MT_NODE_LIST, P_PARSER)
	MEMBER(op_def, out,     node_list_t, MT_NODE_LIST, P_PARSER)
	MEMBER(op_def, options, node_list_t, MT_NODE_LIST, P_PARSER)
	MEMBER(op_def, body,    node_list_t, MT_NODE_LIST, P_PARSER)
	
	MEMBER(op_def, precedence, int64_t, MT_INT, P_PARSER)
	MEMBER(op_def, assoc,      int64_t, MT_INT, P_PARSER)  // actually op_assoc_t
END(op_def)

BEGIN(op_buildin, OP_BUILDIN, NC_NS | NC_NAME | NC_BUILDIN)
END(op_buildin)


// Represents general key-value node for named expressions (options list, named call args,
// hash elements, etc.)
BEGIN(arg, ARG, NC_NAME | NC_VALUE | NC_STORAGE)
	// name can be 0, NULL in case the arg is unnamed
	MEMBER(arg, expr, node_p,  MT_NODE, P_PARSER)
END(arg)


//
// Satements
//

BEGIN(scope, SCOPE, NC_NS)
	MEMBER(scope, stmts, node_list_t, MT_NODE_LIST, P_PARSER)
END(scope)

BEGIN(var, VAR, 0)
	MEMBER(var, type_expr, node_p,      MT_NODE,      P_PARSER)
	MEMBER(var, bindings,  node_list_t, MT_NODE_LIST, P_PARSER)
END(var)

BEGIN(binding, BINDING, NC_NAME | NC_VALUE | NC_STORAGE)
	MEMBER(binding, value, node_p, MT_NODE, P_PARSER)
END(binding)

BEGIN(if_stmt, IF_STMT, NC_NS)
	// Namespace of this node should only be used for lookups that come
	// from the true_case.
	MEMBER(if_stmt, cond,       node_p,      MT_NODE,      P_PARSER)
	MEMBER(if_stmt, true_case,  node_list_t, MT_NODE_LIST, P_PARSER)
	MEMBER(if_stmt, false_case, node_list_t, MT_NODE_LIST, P_PARSER)
END(if_stmt)

BEGIN(while_stmt, WHILE_STMT, NC_NS)
	MEMBER(while_stmt, cond, node_p,      MT_NODE,      P_PARSER)
	MEMBER(while_stmt, body, node_list_t, MT_NODE_LIST, P_PARSER)
END(while_stmt)

BEGIN(return_stmt, RETURN_STMT, 0)
	MEMBER(return_stmt, args, node_list_t, MT_NODE_LIST, P_PARSER)
END(return_stmt)


//
// Literals
//

BEGIN(id, ID, NC_VALUE)
	MEMBER(id, name, str_t,  MT_STR,  P_PARSER)
END(id)

BEGIN(intl, INTL, NC_VALUE)
	MEMBER(intl, value, int64_t, MT_INT,  P_PARSER)
END(intl)

BEGIN(strl, STRL, NC_VALUE)
	MEMBER(strl, value, str_t,  MT_STR,  P_PARSER)
END(strl)


//
// Expressions
//

BEGIN(unary_op, UNARY_OP, NC_VALUE)
	MEMBER(unary_op, name, str_t,  MT_STR,  P_PARSER)
	MEMBER(unary_op, op,   node_p, MT_NODE, P_PARSER)
	MEMBER(unary_op, arg,  node_p, MT_NODE, P_PARSER)
END(unary_op)

BEGIN(uops, UOPS, 0)
	// uops nodes are resolved directly after the parser pass so no member for
	// further passes.
	MEMBER(uops, list, node_list_t, MT_NODE_LIST, P_PARSER)
END(uops)

BEGIN(op, OP, NC_VALUE)
	MEMBER(op, def, node_p, MT_NODE, P_PARSER)
	MEMBER(op, a,   node_p, MT_NODE, P_PARSER)
	MEMBER(op, id,  node_p, MT_NODE, P_PARSER)
	MEMBER(op, b,   node_p, MT_NODE, P_PARSER)
END(op)

BEGIN(member, MEMBER, NC_VALUE)
	MEMBER(member, aggregate, node_p, MT_NODE, P_PARSER)
	MEMBER(member, member,    str_t,  MT_STR,  P_PARSER)
END(member)

BEGIN(call, CALL, NC_VALUE)
	MEMBER(call, target_expr, node_p,      MT_NODE,      P_PARSER)
	MEMBER(call, args,        node_list_t, MT_NODE_LIST, P_PARSER)
END(call)

BEGIN(index, INDEX, NC_VALUE)
	MEMBER(index, target_expr, node_p,      MT_NODE,      P_PARSER)
	MEMBER(index, args,        node_list_t, MT_NODE_LIST, P_PARSER)
END(index)


#undef BEGIN
#undef MEMBER
#undef END