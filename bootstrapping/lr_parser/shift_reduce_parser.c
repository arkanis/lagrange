#include <stdbool.h>
#include <stdio.h>
#include "shift_reduce_parser.h"


#define T_ID  'i'
#define T_EOF '$'

#define TOKEN_COUNT 6
uint8_t tokens[TOKEN_COUNT] = { T_ID, '+', '*', '(', ')', T_EOF };

#define NONTERMINAL_COUNT 3
uint8_t nonterminals[NONTERMINAL_COUNT] = { 'E', 'T', 'F' };

lr_rule_t production_table[] = {
	{ "E -> E+T", 3, 0 },
	{ "E -> T",   1, 0 },
	{ "T -> T*F", 3, 1 },
	{ "T -> F",   1, 1 },
	{ "F -> (E)", 3, 2 },
	{ "F -> id",  1, 2 }
};

#define STATE_COUNT 12
lr_action_t action_table[STATE_COUNT][TOKEN_COUNT] = {
	//       |---- id  ----|----  +  ----|----  *  ----|----  (  ----|----  )  ----|---- EOF ----|
	/* 0 */ { shift(5),        error,           error,           shift(4),        error,           error         },
	/* 1 */ { error,           shift(6),        error,           error,           error,           accept        },
	/* 2 */ { error,           reduce(1),       shift(7),        error,           reduce(1),       reduce(1)     },
	/* 3 */ { error,           reduce(3),       reduce(3),       error,           reduce(3),       reduce(3)     },
	/* 4 */ { shift(5),        error,           error,           shift(4),        error,           error         },
	/* 5 */ { error,           reduce(5),       reduce(5),       error,           reduce(5),       reduce(5)     },
	/* 6 */ { shift(5),        error,           error,           shift(4),        error,           error         },
	/* 7 */ { shift(5),        error,           error,           shift(4),        error,           error         },
	/* 8 */ { error,           shift(6),        error,           error,           shift(11),       error         },
	/* 9 */ { error,           reduce(0),       shift(7),        error,           reduce(0),       reduce(0)     },
	/*10 */ { error,           reduce(2),       reduce(2),       error,           reduce(2),       reduce(2)     },
	/*11 */ { error,           reduce(4),       reduce(4),       error,           reduce(4),       reduce(4)     }
};

ssize_t goto_table[STATE_COUNT][NONTERMINAL_COUNT] = {
	/* 0 */ {  1,  2,  3 },
	/* 1 */ { -1, -1, -1 },
	/* 2 */ { -1, -1, -1 },
	/* 3 */ { -1, -1, -1 },
	/* 4 */ {  8,  2,  3 },
	/* 5 */ { -1, -1, -1 },
	/* 6 */ { -1,  9,  3 },
	/* 7 */ { -1, -1, 10 },
	/* 8 */ { -1, -1, -1 },
	/* 9 */ { -1, -1, -1 },
	/*10 */ { -1, -1, -1 },
	/*11 */ { -1, -1, -1 }
};


lr_action_p get_action(size_t state, uint8_t token){
	size_t token_idx = 0;
	while(tokens[token_idx] != token)
		token_idx++;
	
	return &action_table[state][token_idx];
}

lr_rule_p get_production(size_t prod_idx){
	return &production_table[prod_idx];
}

size_t get_goto_state(size_t state_idx, size_t nonterminal_idx){
	return goto_table[state_idx][nonterminal_idx];
}

void report_error(size_t state, uint8_t token){
	printf("could not parse input, got %c, exptected:", token);
	for(size_t i = 0; i < TOKEN_COUNT; i++) {
		lr_action_p action = &action_table[state][i];
		if (action->type != LR_ERROR)
			printf(" %c", tokens[i]);
	}
	printf("\n");
}

int main(int argc, char** argv){
	uint8_t input_tokens[] = { T_ID, '*', T_ID, '+', '(', T_ID, '+', T_ID, ')', T_EOF };
	size_t input_idx = 0;
	
	size_t state_stack[32] = {0};
	size_t stack_idx = 0;
	
	uint8_t consume(){
		uint8_t token = input_tokens[input_idx];
		input_idx++;
		return token;
	}
	
	uint8_t peek(){
		return input_tokens[input_idx];
	}
	
	void push_state(size_t state){
		stack_idx++;
		state_stack[stack_idx] = state;
	}
	
	void pop_states(size_t count){
		stack_idx -= count;
	}
	
	while(true){
		size_t current_state = state_stack[stack_idx];
		printf("current state: %zu\n", current_state);
		
		uint8_t token = peek();
		lr_action_p action = get_action(current_state, token);
		
		switch(action->type){
			case LR_ERROR:
				report_error(current_state, token);
				return 3;
				break;
			case LR_SHIFT:
				push_state(action->shift_state);
				consume();
				/*
				if (peek() == T_EOF)
					return 2;
				*/
				break;
			case LR_ACCEPT:
				printf("we're done :)\n");
				return 0;
				break;
			case LR_REDUCE: {
				lr_rule_p production = get_production(action->production_idx);
				pop_states(production->symbol_count);
				
				size_t current_top_state = state_stack[stack_idx];
				size_t new_state = get_goto_state(current_top_state, production->nonterminal_idx);
				
				if (new_state == -1) {
					printf("table deeply broken");
					return 1;
				}
				
				push_state(new_state);
				} break;
		}
	}
}