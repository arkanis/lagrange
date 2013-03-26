#include <stdio.h>
#include <string.h>
#include "rule.h"

void main(){
	rule_state_t rs = (rule_state_t){
		.instructions = (rule_instr_t[]){
			{OP_CONSUME, 'h'},
			{OP_CONSUME, 'e'},
			{OP_CONSUME, 'l'},
			{OP_CONSUME, 'l'},
			{OP_CONSUME, 'o'}
		},
		.instruction_count = 5,
		.next_instr_idx = 0
	};
	
	char buffer[100];
	
	scanf(" %100[^\n]", buffer);
	printf("read: %s\n", buffer);
	
	for(size_t i = 0; i < strlen(buffer); i++){
		if ( rule_advance(&rs, buffer[i]) )
			printf("%zu: %c matched\n", i, buffer[i]);
		else
			printf("%zu: %c NOT matched\n", i, buffer[i]);
	}
}