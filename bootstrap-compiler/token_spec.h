//#define TOKEN(id, free_expr, desc, print) free_expr;
// id, free_expr
// free_expr: expression that frees all memory associated with the token. The
//   variable "t" of type token_p. Use "_" as a placeholder for no action.

TOKEN(T_COMMENT, _ )
TOKEN(T_WS,      _ )
TOKEN(T_WSNL,    _ )

TOKEN(T_STR, str_free(&t->str_val) )
TOKEN(T_INT, _                     )