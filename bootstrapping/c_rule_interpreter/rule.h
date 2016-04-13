#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


// Instruction: op ( char | range-count range ... )
#define OP_NULL              0
// Consume one specified char
#define OP_CONS_CHAR         1
// Consumes one char in ranges
#define OP_CONS_RANGE        2
// Consumes until chars until one char is found
#define OP_CONS_UNTIL_CHAR   3
// Consumes until chars until a char in ranges is found
#define OP_CONS_UNTIL_RANGE  4
// Consumes while chars match one char
#define OP_CONS_WHILE_CHAR   5
// Consumes while chars match ranges
#define OP_CONS_WHILE_RANGE  6

/*
#define OP_MATCH
#define OP_FAIL
*/

/* Example:
(uint8_t[]){
	OP_CONS_RANGE, 2, '0', '9', 'a', 'z'
}
*/

typedef struct {
	size_t code_size;
	uint8_t code[];
} rule_t, *rule_p;

typedef struct {
	rule_p rule;
	ssize_t next_instr_offset;
} rule_state_t, *rule_state_p;

rule_p rule_new(uint8_t code[]);
void   rule_destroy(rule_p rule);
bool   rule_advance(rule_state_p state, uint8_t val);
