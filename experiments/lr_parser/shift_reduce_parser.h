#pragma once

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>


#define LR_SHIFT  1
#define LR_REDUCE 2
#define LR_ACCEPT 3
#define LR_ERROR  4

typedef struct {
	uint8_t type;
	union {
		size_t shift_state;
		size_t production_idx;
	};
} lr_action_t, *lr_action_p, action_t, *action_p;

#define shift(state)     { LR_SHIFT, .shift_state = state }
#define reduce(prod_idx) { LR_REDUCE, .production_idx = prod_idx }
#define accept           { LR_ACCEPT }
#define error            { LR_ERROR }


typedef struct {
	char* name;
	size_t symbol_count;
	size_t nonterminal_idx;
} lr_rule_t, *lr_rule_p;