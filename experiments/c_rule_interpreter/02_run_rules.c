#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rule.h"


int main(){
	rule_p r_if = rule_new((uint8_t[]){
		OP_CONS_CHAR, 'i',
		OP_CONS_CHAR, 'f',
		OP_NULL
	});
	
	rule_p r_while = rule_new((uint8_t[]){
		OP_CONS_CHAR, 'w',
		OP_CONS_CHAR, 'h',
		OP_CONS_CHAR, 'i',
		OP_CONS_CHAR, 'l',
		OP_CONS_CHAR, 'e',
		OP_NULL
	});
	
	rule_p r_digit = rule_new((uint8_t[]){
		OP_CONS_WHILE_RANGE, 1, '0', '9',
		OP_NULL
	});
	
	rule_p r_id = rule_new((uint8_t[]){
		OP_CONS_RANGE, 2, 'a', 'z', 'A', 'Z',
		OP_CONS_WHILE_RANGE, 3, '0', '9', 'a', 'z', 'A', 'Z',
		OP_NULL
	});
	
	
	const size_t rule_count = 4;
	rule_state_t rule_states[4] = {
		{r_if,    0},
		{r_while, 0},
		{r_digit, 0},
		{r_id,    0}
	};
	
	
	char buffer[100];
	scanf(" %100[^\n]", buffer);
	
	for(size_t char_idx = 0; char_idx < strlen(buffer); char_idx++){
		printf("[%2zu] %c:", char_idx, buffer[char_idx]);
		
		for(size_t rs_idx = 0; rs_idx < rule_count; rs_idx++){
			rule_state_p rs = &rule_states[rs_idx];
			if (rs->next_instr_offset == -1) {
				printf("  ");
				continue;
			}
			
			if ( rule_advance(rs, buffer[char_idx]) ) {
				printf(" %c", buffer[char_idx]);
			} else {
				printf(" -");
				rs->next_instr_offset = -1;
			}
		}
		
		printf("\n");
	}
	
	
	rule_destroy(r_if);
	rule_destroy(r_while);
	rule_destroy(r_digit);
	rule_destroy(r_id);
	
	return 0;
}