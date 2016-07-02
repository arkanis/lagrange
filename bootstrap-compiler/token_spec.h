//#define TOKEN(id, free_expr, desc, print) free_expr;
// id, free_expr
// free_expr: expression that frees all memory associated with the token. The
//   variable "t" of type token_p. Use "_" as a placeholder for no action.

TOKEN(T_COMMENT,        _,        _ )  // // and /*
TOKEN(T_WS,             _,        _ )
TOKEN(T_WSNL,           _,        _ )

TOKEN(T_STR,            _,        str_free(&t->str_val) )
TOKEN(T_INT,            _,        _                     )
TOKEN(T_ID,             _,        _                     )

TOKEN(T_CBO,            _,        _                   ) // {
TOKEN(T_CBC,            _,        _                   ) // }
TOKEN(T_RBO,            _,        _                   ) // (
TOKEN(T_RBC,            _,        _                   ) // )
TOKEN(T_COMMA,          _,        _                   ) // ,
	
// Tokens for unary and binary operators
TOKEN(T_ADD,            _,        _              ) // +
TOKEN(T_ADD_ASSIGN,     _,        _              ) // +=
TOKEN(T_SUB,            _,        _              ) // -
TOKEN(T_SUB_ASSIGN,     _,        _              ) // -=
TOKEN(T_MUL,            _,        _              ) // *
TOKEN(T_MUL_ASSIGN,     _,        _              ) // *=
TOKEN(T_DIV,            _,        _              ) // /
TOKEN(T_DIV_ASSIGN,     _,        _              ) // /=
TOKEN(T_MOD,            _,        _              ) // %
TOKEN(T_MOD_ASSIGN,     _,        _              ) // %=

TOKEN(T_LT,             _,        _ ) // <
TOKEN(T_LE,             _,        _ ) // <=
TOKEN(T_SL,             _,        _ ) // <<
TOKEN(T_SL_ASSIGN,      _,        _ ) // <<=
TOKEN(T_GT,             _,        _ ) // >
TOKEN(T_GE,             _,        _ ) // >=
TOKEN(T_SR,             _,        _ ) // >>
TOKEN(T_SR_ASSIGN,      _,        _ ) // >>=

TOKEN(T_BIN_AND,        _,        _ ) // &
TOKEN(T_BIN_AND_ASSIGN, _,        _ ) // &=    && becomes T_AND
TOKEN(T_BIN_OR,         _,        _ ) // |
TOKEN(T_BIN_OR_ASSIGN,  _,        _ ) // |=    || becomes T_OR
TOKEN(T_BIN_XOR,        _,        _ ) // ^
TOKEN(T_BIN_XOR_ASSIGN, _,        _ ) // ^=

TOKEN(T_ASSIGN,         _,        _ ) // =
TOKEN(T_EQ,             _,        _ ) // ==
TOKEN(T_NEQ,            _,        _ ) // !=    ! becomes T_NOT
TOKEN(T_PERIOD,         _,        _ ) // .
TOKEN(T_COMPL,          _,        _ ) // ~

// Keywords
TOKEN(T_NOT,            "not",    _ )
TOKEN(T_AND,            "and",    _ )
TOKEN(T_OR,             "or",     _ )
TOKEN(T_VAR,            "var",    _ )
TOKEN(T_IF,             "if",     _ )
TOKEN(T_THEN,           "then",   _ )
TOKEN(T_ELSE,           "else",   _ )
TOKEN(T_WHILE,          "while",  _ )
TOKEN(T_DO,             "do",     _ )
TOKEN(T_FUNC,           "func",   _ )
TOKEN(T_IN,             "in",     _ )
TOKEN(T_OUT,            "out",    _ )
TOKEN(T_RETURN,         "return", _ )

TOKEN(T_ERROR,          _,        _ )
TOKEN(T_EOF,            _,        _ )