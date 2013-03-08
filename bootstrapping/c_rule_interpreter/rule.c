#include <stdio.h>
#include "rule.h"


bool rule_advance(rule_state_p rs, uint8_t val) {
	if (rs->next_instr_idx >= rs->instruction_count)
		return false;
	
	rule_instr_t instr = rs->instructions[rs->next_instr_idx];
	switch(instr.op) {
		case OP_CONSUME:
			rs->next_instr_idx++;
			if (instr.val == val)
				return true;
			else
				return false;
			break;
		
		case OP_UNTIL:
			if (instr.val == val)
				rs->next_instr_idx++;
			return true;
			break;
	}
	
	fprintf(stderr, "invalid instruction!\n");
	return false;
}