#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rule.h"

#define RULE_COUNT 5

void main(){
	rule_state_p rules[RULE_COUNT] = {NULL};
	
	rules[0] = malloc(sizeof(rule_state_t));
	*rules[0] = (rule_state_t){
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
	
	rules[1] = malloc(sizeof(rule_state_t));
	*rules[1] = (rule_state_t){
		.instructions = (rule_instr_t[]){
			{OP_CONSUME, '"'},
			{OP_UNTIL,   '"'}
		},
		.instruction_count = 2,
		.next_instr_idx = 0
	};
	
	
	char buffer[100];
	while( scanf(" %100[^\n]", buffer) != EOF ) {
		
		for(size_t i = 0; i < strlen(buffer); i++){
			printf("[%zu] %c:", i, buffer[i]);
			
			for(size_t ri = 0; ri < RULE_COUNT; ri++){
				rule_state_p rs = rules[ri];
				if (rs == NULL) {
					printf("  ");
					continue;
				}
				
				if ( rule_advance(rs, buffer[i]) ) {
					printf(" %c", buffer[i]);
				} else {
					printf(" -");
					free(rs);
					rules[ri] = NULL;
				}
			}
			
			printf("\n");
		}
	}
	
	for(size_t i = 0; i < RULE_COUNT; i++)
		free(rules[i]);
}