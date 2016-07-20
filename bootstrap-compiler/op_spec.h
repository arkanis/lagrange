#ifndef UNARY_OP
	#define UNARY_OP(token, name)
#endif
#ifndef BINARY_OP
	#define BINARY_OP(token, name)
#endif


UNARY_OP(T_ADD,   plus)
UNARY_OP(T_SUB,   minus)
UNARY_OP(T_NOT,   not)
UNARY_OP(T_COMPL, compl)


BINARY_OP(T_MUL,      mul)
BINARY_OP(T_DIV,      div)
BINARY_OP(T_MOD,      mod)
BINARY_OP(T_ADD,      add)
BINARY_OP(T_SUB,      sub)
BINARY_OP(T_SL,       shift_left)
BINARY_OP(T_SR,       shift_right)
BINARY_OP(T_LT,       lt)
BINARY_OP(T_LE,       le)
BINARY_OP(T_GT,       gt)
BINARY_OP(T_GE,       ge)
BINARY_OP(T_EQ,       eq)
BINARY_OP(T_NEQ,      neq)
BINARY_OP(T_BIT_AND,  bit_and)
BINARY_OP(T_BIT_XOR,  bit_xor)
BINARY_OP(T_BIT_OR,   bit_or)
BINARY_OP(T_AND,      and)
BINARY_OP(T_OR,       or)

BINARY_OP(T_ASSIGN,          assign)
BINARY_OP(T_MUL_ASSIGN,      mul_assign)
BINARY_OP(T_DIV_ASSIGN,      div_assign)
BINARY_OP(T_MOD_ASSIGN,      mod_assign)
BINARY_OP(T_ADD_ASSIGN,      add_assign)
BINARY_OP(T_SUB_ASSIGN,      sub_assign)
BINARY_OP(T_SL_ASSIGN,       shift_left_assign)
BINARY_OP(T_SR_ASSIGN,       shift_right_assign)
BINARY_OP(T_BIT_AND_ASSIGN,  bit_and_assign)
BINARY_OP(T_BIT_XOR_ASSIGN,  bit_xor_assign)
BINARY_OP(T_BIT_OR_ASSIGN,   bit_or_assign)


#undef UNARY_OP
#undef BINARY_OP