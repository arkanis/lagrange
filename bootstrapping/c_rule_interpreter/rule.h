#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


// Rule byte code
typedef struct {
	uint8_t op;
	uint8_t val;
} rule_instr_t, *rule_instr_p;

#define OP_CONSUME 1
// Only continue with the next instruction if we found the required char (loop _until_ that char)
#define OP_UNTIL   2


// Rule state
typedef struct {
	size_t instruction_count;
	rule_instr_p instructions;
	size_t next_instr_idx;
} rule_state_t, *rule_state_p;

bool rule_advance(rule_state_p rs, uint8_t val);