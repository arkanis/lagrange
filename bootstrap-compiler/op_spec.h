#ifndef UNARY_OP
	#define UNARY_OP(token, id, name)
#endif
#ifndef SPECIAL_OP
	#define SPECIAL_OP(token, id, name)
#endif
#ifndef BINARY_OP
	#define BINARY_OP(token, id, name)
#endif


UNARY_OP(T_ADD,    OP_PLUS,  plus)
UNARY_OP(T_SUB,    OP_MINUS, minus)
UNARY_OP(T_NOT,    OP_NOT,   not)
UNARY_OP(T_COMPL,  OP_COMPL, compl)

SPECIAL_OP(T_PERIOD,   OP_PERIOD,      _)
SPECIAL_OP(T_ID,       OP_USERDEFINED, _)

BINARY_OP(T_MUL,      OP_MUL,         mul)
BINARY_OP(T_DIV,      OP_DIV,         div)
BINARY_OP(T_MOD,      OP_MOD,         mod)
BINARY_OP(T_ADD,      OP_ADD,         add)
BINARY_OP(T_SUB,      OP_SUB,         sub)
BINARY_OP(T_SL,       OP_SL,          shift_left)
BINARY_OP(T_SR,       OP_SR,          shift_right)
BINARY_OP(T_LT,       OP_LT,          lt)
BINARY_OP(T_LE,       OP_LE,          le)
BINARY_OP(T_GT,       OP_GT,          gt)
BINARY_OP(T_GE,       OP_GE,          ge)
BINARY_OP(T_EQ,       OP_EQ,          eq)
BINARY_OP(T_NEQ,      OP_NEQ,         neq)
BINARY_OP(T_BIT_AND,  OP_BIT_AND,     bit_and)
BINARY_OP(T_BIT_XOR,  OP_BIT_XOR,     bit_xor)
BINARY_OP(T_BIT_OR,   OP_BIT_OR,      bit_or)
BINARY_OP(T_AND,      OP_AND,         and)
BINARY_OP(T_OR,       OP_OR,          or)

BINARY_OP(T_ASSIGN,          OP_ASSIGN,          assign)
BINARY_OP(T_MUL_ASSIGN,      OP_MUL_ASSIGN,      mul_assign)
BINARY_OP(T_DIV_ASSIGN,      OP_DIV_ASSIGN,      div_assign)
BINARY_OP(T_MOD_ASSIGN,      OP_MOD_ASSIGN,      mod_assign)
BINARY_OP(T_ADD_ASSIGN,      OP_ADD_ASSIGN,      add_assign)
BINARY_OP(T_SUB_ASSIGN,      OP_SUB_ASSIGN,      sub_assign)
BINARY_OP(T_SL_ASSIGN,       OP_SL_ASSIGN,       shift_left_assign)
BINARY_OP(T_SR_ASSIGN,       OP_SR_ASSIGN,       shift_right_assign)
BINARY_OP(T_BIT_AND_ASSIGN,  OP_BIT_AND_ASSIGN,  bit_and_assign)
BINARY_OP(T_BIT_OR_ASSIGN,   OP_BIT_XOR_ASSIGN,  bit_xor_assign)
BINARY_OP(T_BIT_XOR_ASSIGN,  OP_BIT_OR_ASSIGN,   bit_or_assign)


#undef UNARY_OP
#undef SPECIAL_OP
#undef BINARY_OP