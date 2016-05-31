#pragma once
#include "lexer.h"
#include "ast.h"

typedef struct parser_s parser_t, *parser_p;
typedef node_p (*parser_rule_func_t)(parser_p parser);

node_p parse(token_list_p tokens, parser_rule_func_t rule);

node_p parse_module(parser_p parser);
node_p parse_func_def(parser_p parser);
node_p parse_stmt(parser_p parser);
node_p parse_expr(parser_p parser);