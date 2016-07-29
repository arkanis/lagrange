//#define TOKEN(id, free_expr, desc, print) free_expr;
// id, keyword, free_expr
// free_expr: expression that frees all memory associated with the token. The
//   variable "t" of type token_p. Use "_" as a placeholder for no action.

//    id                keyword   free_expr              print
TOKEN(T_COMMENT,        _,        _,                     _ )  // // and /*
TOKEN(T_WS,             _,        _,                     _ )
TOKEN(T_WSNL,           _,        _,                     _ )

TOKEN(T_STR,            _,        str_free(&t->str_val), _ )
TOKEN(T_INT,            _,        _,                     _ )
TOKEN(T_ID,             _,        _,                     _ )

TOKEN(T_CBO,            _,        _,                     "{"     )
TOKEN(T_CBC,            _,        _,                     "}"     )
TOKEN(T_RBO,            _,        _,                     "("     )
TOKEN(T_RBC,            _,        _,                     ")"     )
TOKEN(T_SBO,            _,        _,                     "["     )
TOKEN(T_SBC,            _,        _,                     "]"     )
TOKEN(T_COMMA,          _,        _,                     "comma" )
TOKEN(T_SEMI,           _,        _,                     ";"     )
TOKEN(T_COLON,          _,        _,                     ":"     )
	
// Tokens for unary and binary operators
TOKEN(T_ADD,            _,        _,                     "+"  )
TOKEN(T_ADD_ASSIGN,     _,        _,                     "+=" )
TOKEN(T_SUB,            _,        _,                     "-"  )
TOKEN(T_SUB_ASSIGN,     _,        _,                     "-=" )
TOKEN(T_MUL,            _,        _,                     "*"  )
TOKEN(T_MUL_ASSIGN,     _,        _,                     "*=" )
TOKEN(T_DIV,            _,        _,                     "/"  )
TOKEN(T_DIV_ASSIGN,     _,        _,                     "/=" )
TOKEN(T_MOD,            _,        _,                     "%"  )
TOKEN(T_MOD_ASSIGN,     _,        _,                     "%=" )

TOKEN(T_LT,             _,        _,                     "<"   )
TOKEN(T_LE,             _,        _,                     "<="  )
TOKEN(T_SL,             _,        _,                     "<<"  )
TOKEN(T_SL_ASSIGN,      _,        _,                     "<<=" )
TOKEN(T_GT,             _,        _,                     ">"   )
TOKEN(T_GE,             _,        _,                     ">="  )
TOKEN(T_SR,             _,        _,                     ">>"  )
TOKEN(T_SR_ASSIGN,      _,        _,                     ">>=" )

TOKEN(T_BIT_AND,        _,        _,                     "&"  )
TOKEN(T_BIT_AND_ASSIGN, _,        _,                     "&=" )  // && becomes T_AND
TOKEN(T_BIT_OR,         _,        _,                     "|"  )
TOKEN(T_BIT_OR_ASSIGN,  _,        _,                     "|=" )  // || becomes T_OR
TOKEN(T_BIT_XOR,        _,        _,                     "^"  )
TOKEN(T_BIT_XOR_ASSIGN, _,        _,                     "^=" )

TOKEN(T_ASSIGN,         _,        _,                     "="  )
TOKEN(T_EQ,             _,        _,                     "==" )
TOKEN(T_NEQ,            _,        _,                     "!=" )  // ! becomes T_NOT
TOKEN(T_PERIOD,         _,        _,                     "."  )
TOKEN(T_COMPL,          _,        _,                     "~"  )

// Keywords
TOKEN(T_NOT,            "not",      _,                   "not"      )
TOKEN(T_AND,            "and",      _,                   "and"      )
TOKEN(T_OR,             "or",       _,                   "or"       )
TOKEN(T_VAR,            "var",      _,                   "var"      )
TOKEN(T_IF,             "if",       _,                   "if"       )
TOKEN(T_THEN,           "then",     _,                   "then"     )
TOKEN(T_ELSE,           "else",     _,                   "else"     )
TOKEN(T_WHILE,          "while",    _,                   "while"    )
TOKEN(T_DO,             "do",       _,                   "do"       )
TOKEN(T_END,            "end",      _,                   "end"      )
TOKEN(T_FUNC,           "func",     _,                   "func"     )
TOKEN(T_OPERATOR,       "operator", _,                   "operator" )
TOKEN(T_IN,             "in",       _,                   "in"       )
TOKEN(T_OUT,            "out",      _,                   "out"      )
TOKEN(T_OPTIONS,        "options",  _,                   "options"  )
TOKEN(T_RETURN,         "return",   _,                   "return"   )

// Uses only static error messages, so no free necessary
TOKEN(T_ERROR,          _,        _,                     _     )
TOKEN(T_EOF,            _,        _,                     "EOF" )