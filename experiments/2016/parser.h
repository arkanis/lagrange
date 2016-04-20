#pragma once
#include "lexer.h"
#include "ast.h"

typedef struct parser_s parser_t, *parser_p;
typedef node_p (*parser_rule_func_t)(parser_p parser);

node_p parse(tokenized_file_p file, parser_rule_func_t rule);

node_p parse_expr(parser_p parser);