#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rule.h"

// TODO: handle null bytes in instructions (e.g. for unicode or uint32_t chars)
rule_p rule_new(uint8_t code[]){
	size_t code_size = 0;
	while(code[code_size] != 0)
		code_size++;
	
	rule_p rule = malloc(offsetof(rule_t, code) + code_size);
	rule->code_size = code_size;
	memcpy(rule->code, code, code_size);
	
	return rule;
}

void rule_destroy(rule_p rule){
	free(rule);
}

bool rule_advance(rule_state_p state, uint8_t val) {
	if ( !(state->next_instr_offset < state->rule->code_size) )
		return false;
	
	uint8_t* instr = state->rule->code + state->next_instr_offset;
	switch(instr[0]) {
		case OP_CONS_CHAR:
		case OP_CONS_UNTIL_CHAR:
		case OP_CONS_WHILE_CHAR:
			{
				uint8_t instr_val = instr[1];
				size_t instr_size = 2;
				
				switch (instr[0]) {
					case OP_CONS_CHAR:
						state->next_instr_offset += instr_size;
						return (val == instr_val);
					case OP_CONS_UNTIL_CHAR:
						if (val == instr_val)
							state->next_instr_offset += instr_size;
						return true;
					case OP_CONS_WHILE_CHAR:
						if (val != instr_val)
							state->next_instr_offset += instr_size;
						return true;
				}
				
			}
			break;
			
		case OP_CONS_RANGE:
		case OP_CONS_UNTIL_RANGE:
		case OP_CONS_WHILE_RANGE:
			{
				uint8_t instr_range_count = instr[1];
				size_t instr_size = 2 + instr_range_count * 2;
				
				
				switch (instr[0]) {
					case OP_CONS_RANGE:
						state->next_instr_offset += instr_size;
						
						for(size_t i = 0; i < instr_range_count; i++) {
							uint8_t min_val = instr[2 + i * 2];
							uint8_t max_val = instr[2 + i * 2 + 1];
							if ( val >= min_val && val <= max_val )
								return true;
						}
						
						return false;
						
					case OP_CONS_UNTIL_RANGE:
						for(size_t i = 0; i < instr_range_count; i++) {
							uint8_t min_val = instr[2 + i * 2];
							uint8_t max_val = instr[2 + i * 2 + 1];
							if ( val >= min_val && val <= max_val ) {
								state->next_instr_offset += instr_size;
								return true;
							}
						}
						
						return true;
						
					case OP_CONS_WHILE_RANGE:
						for(size_t i = 0; i < instr_range_count; i++) {
							uint8_t min_val = instr[2 + i * 2];
							uint8_t max_val = instr[2 + i * 2 + 1];
							if ( val >= min_val && val <= max_val )
								return true;
						}
						
						state->next_instr_offset += instr_size;
						return true;
				}
			}
			break;
	}
	
	fprintf(stderr, "invalid instruction!\n");
	return false;
}