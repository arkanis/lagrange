#include <string.h>
#include <stdio.h>

#include "grammar.h"
#include "shift_reduce_parser.h"


// Index relative to the first symbol (nonterminals after terminals!)
ssize_t symbol_index(uint32_t symbol, grammar_p grammar){
	for(size_t i = grammar->terminal_count; i < grammar->terminal_count + grammar->nonterminal_count; i++) {
		if (grammar->symbols[i] == symbol)
			return i;
	}
	
	return -1;
}

// Returns the index of the nonterminal relative to the first nonterminal
ssize_t nonterminal_index(uint32_t symbol, grammar_p g){
	ssize_t sym_index = symbol_index(symbol, g);
	if (sym_index == -1)
		return -1;
	
	return sym_index - g->terminal_count;
}

void build_first_sets(grammar_p g){
	g->first_count = g->nonterminal_count;
	g->first_sets = malloc(g->first_count * sizeof(symbol_set_p));
	
	for(size_t i = 0; i < g->first_count; i++) {
		g->first_sets[i] = symbol_set_new();
	}
	
	while(true) {
		size_t appended = 0;
		
		for(size_t i = 0; i < g->length; i++) {
			rule_p rule = &g->rules[i];
			
			uint32_t rule_head = rule->symbols[0];
			ssize_t rule_head_index = nonterminal_index(rule_head, g);
			if (rule_head_index == -1)
				printf("nonterminal not found: %c !!!\n", nt_to_char(rule_head));

			
			symbol_set_p rule_head_first_set = symbol_set_new();
			
			bool epsilon_everywhere = true;
			for(size_t j = 1; j < rule->length; j++) {
				uint32_t jth_symbol = rule->symbols[j];
				
				if ( is_terminal(jth_symbol) ) {
					symbol_set_add(rule_head_first_set, jth_symbol);
					epsilon_everywhere = false;
					break;
				}
				
				// jth_symbol is a nonterminal
				size_t jth_symbol_index = nonterminal_index(jth_symbol, g);
				symbol_set_p jth_first_set = g->first_sets[jth_symbol_index];
				symbol_set_append(rule_head_first_set, jth_first_set);
				
				// Only continue with the next symbol of this rule if the jth sym allows for expsilon
				if ( ! symbol_set_contains(jth_first_set, T_EPSILON) ) {
					epsilon_everywhere = false;
					break;
				}
			}
			
			if (!epsilon_everywhere)
				rule_head_first_set = symbol_set_without(rule_head_first_set, T_EPSILON);
			
			appended += symbol_set_append(g->first_sets[rule_head_index], rule_head_first_set);
			symbol_set_destroy(rule_head_first_set);
		}
		
		// If the follow sets haven't changed we're done
		if (appended == 0)
			break;
	}
}

void print_first_sets(grammar_p g){
	printf("first sets:\n");
	for(size_t i = 0; i < g->first_count; i++) {
		printf("  %c:", nt_to_char(g->symbols[g->terminal_count + i]) );
		
		void iterator(uint32_t symbol){
			printf(" %c", symbol);
		}
		symbol_set_iterate(g->first_sets[i], iterator);
		
		printf("\n");
	}
}

symbol_set_p first(uint32_t* symbols, size_t symbol_count, grammar_p g){
	/*
	uint32_t symbol = symbols[0];
	symbol_set_p set = symbol_set_new();
	
	if ( is_terminal(symbol) ) {
		symbol_set_add(set, symbol);
		return set;
	}
	*/
	symbol_set_p set = symbol_set_new();
	
	bool epsilon_everywhere = true;
	for(size_t i = 0; i < symbol_count; i++) {
		uint32_t ith_symbol = symbols[i];
		
		if ( is_terminal(ith_symbol) ) {
			symbol_set_add(set, ith_symbol);
			epsilon_everywhere = false;
			break;
		}
		
		// TODO: even more memory leaks here...  maybe, not sure
		symbol_set_p ith_first_set = g->first_sets[nonterminal_index(ith_symbol, g)];
		symbol_set_append(set, ith_first_set);
		
		if ( ! symbol_set_contains(ith_first_set, T_EPSILON) ) {
			epsilon_everywhere = false;
			break;
		}
	}
	
	// TODO: memory leaks here...
	if ( !epsilon_everywhere )
		set = symbol_set_without(set, T_EPSILON);
	
	return set;
}


void build_follow_sets(grammar_p g){
	// The follow set contains a set of following terminals for all nonterminals.
	// The follow set for the artifical start symbol is never used so we don't include it here.
	g->follow_count = g->nonterminal_count;
	g->follow_sets = malloc(g->follow_count * sizeof(symbol_set_p));
	
	for(size_t i = 0; i < g->follow_count; i++) {
		g->follow_sets[i] = symbol_set_new();
	}
	
	ssize_t start_nonterminal_index = nonterminal_index(nt('S'), g);
	if (start_nonterminal_index == -1)
		printf("nonterminal not found: %c !!!\n", nt_to_char(start_nonterminal_index));
	
	symbol_set_add( g->follow_sets[start_nonterminal_index], '$' );
	
	while(true) {
		size_t appended = 0;
		
		for(size_t i = 0; i < g->length; i++) {
			rule_p rule = &g->rules[i];
			
			for(size_t j = 1; j < rule->length; j++) {
				uint32_t rule_symbol = rule->symbols[j];
				
				if ( is_terminal(rule_symbol) )
					continue;
				
				// Now we have the nonterminals of all rules
				ssize_t rule_symbol_index = nonterminal_index(rule_symbol, g);
				bool nonterminal_is_last_in_rule = (j == rule->length - 1);
				
				bool epsilon_in_first_set = false;
				if ( ! nonterminal_is_last_in_rule ) {
					symbol_set_p first_set_of_rest = first(rule->symbols + j + 1, rule->length - j - 1, g);
					symbol_set_p first_set_without_epsilon = symbol_set_without(first_set_of_rest, T_EPSILON);
					appended += symbol_set_append( g->follow_sets[rule_symbol_index], first_set_without_epsilon );
					
					epsilon_in_first_set = symbol_set_contains(first_set_of_rest, T_EPSILON);
				}
				
				if ( epsilon_in_first_set || nonterminal_is_last_in_rule ) {
					uint32_t rule_head = rule->symbols[0];
					ssize_t rule_head_index = nonterminal_index(rule_head, g);
					if (rule_head_index == -1)
						printf("nonterminal not found: %c !!!\n", nt_to_char(rule_head));
					
					appended += symbol_set_append( g->follow_sets[rule_symbol_index], g->follow_sets[rule_head_index] );
				}
			}
		}
		
		// If the follow sets haven't changed we're done
		if (appended == 0)
			break;
	}
}

void print_follow_sets(grammar_p g){
	printf("follow sets:\n");
	for(size_t i = 0; i < g->follow_count; i++) {
		printf("  %c:", nt_to_char(g->symbols[g->terminal_count + i]) );
		
		void iterator(uint32_t symbol){
			printf(" %c", symbol);
		}
		symbol_set_iterate(g->follow_sets[i], iterator);
		
		printf("\n");
	}
}



void closure(item_set_p item_set, grammar_p grammar){
	size_t old_length = item_set->length;
	
	for(size_t i = 0; i < item_set->length; i++) {
		item_p item = &item_set->items[i];
		uint32_t head_symbol = item_next(item);
		
		// Nothing to do for items where the point is not in front of a nonterminal
		if ( ! is_nt(head_symbol) )
			continue;
		
		// Search for rules that produce to the head symbol
		for(size_t j = 0; j < grammar->length; j++) {
			if ( grammar->rules[j].symbols[0] == head_symbol ) {
				item_t new_item = (item_t){
					.rule = &grammar->rules[j],
					.pos = 1
				};
				item_set_add(item_set, new_item);
			}
		}
	}
	
	// If we added new items add items for them until no more items are added
	if (item_set->length > old_length)
		closure(item_set, grammar);
}

// is goto (from dragon book)
item_set_t increment_closure(item_set_p item_set, uint32_t symbol, grammar_p grammar){
	item_set_t new_item_set = {0};
	
	for(size_t i = 0; i < item_set->length; i++) {
		item_p item = &item_set->items[i];
		uint32_t next = item_next(item);
		
		if ( next == -1 || next != symbol )
			continue;
		
		item_set_add(&new_item_set, item_increment(item));
	}
	
	closure(&new_item_set, grammar);
	return new_item_set;
}

states_t grow_lr0_automation(grammar_p grammar, action_p* action_table_out_ptr, ssize_t** goto_table_out_ptr){
	size_t at_rows = 0, at_cols = grammar->terminal_count;
	action_p action_table = NULL;
	ssize_t* goto_table = NULL;
	
	// action table index helper function
	inline size_t ati(size_t row, size_t col){
		return row*at_cols + col;
	}
	
	void add_action_table_row(){
		at_rows++;
		action_table = realloc(action_table, at_cols * at_rows);
		for(size_t i = 0; i < at_cols; i++)
			action_table[ati(at_rows-1, i)] = (action_t){ LR_ERROR };
	}
	
	
	// Build initial item set
	item_set_t initial_item_set = {0};
	item_set_add(&initial_item_set, (item_t){ .rule = &grammar->rules[0], .pos = 1 });
	closure(&initial_item_set, grammar);
	
	// Our list of states, each state is an item set
	size_t state_count = 1;
	item_set_p states = malloc(state_count * sizeof(item_set_t));
	states[0] = initial_item_set;
	
	while(true) {
		size_t old_state_count = state_count;
		
		for(size_t i = 0; i < state_count; i++) {
			for(size_t j = 0; j < grammar->terminal_count + grammar->nonterminal_count; j++) {
				item_set_p state = &states[i];
				
				item_set_t new_state = increment_closure(state, grammar->symbols[j], grammar);
				
				if (new_state.length == 0)
					continue;
				
				bool state_duplicated = false;
				size_t k;
				size_t target_index;
				for(k = 0; k < state_count; k++) {
					if ( item_set_equals(&states[k], &new_state) ) {
						state_duplicated = true;
						break;
					}
				}
				
				// State already present, so use this as index
				if (state_duplicated) {
					target_index = k;
				
				// Create a new state in our state-table
				} else {
					state_count++;
					states = realloc(states, state_count * sizeof(item_set_t));
					states[state_count - 1] = new_state;
					target_index = state_count - 1;

					// also create a row in our action table:
					add_action_table_row();
				}

				// use this information to complete parse table:
				if ( is_terminal(grammar->symbols[j]) ) {
					// action[i, j] = shift(target_index)				
				}				
			}
		}
		
		if ( state_count == old_state_count )
			break;
	}
	
	*action_table_out_ptr = action_table;
	*goto_table_out_ptr = goto_table;
	return (states_t){ state_count, states };
}

void test_increment_closure(grammar_p g){
	item_set_t state = (item_set_t){
		.length = 2,
		.items = (item_t[]){
			{ &g->rules[1], 2 },
			{ &g->rules[5], 3 }
		}
	};
	
	item_set_t new_state = increment_closure(&state, ')', g);
	return;
}

int main(int argc, char** argv){
	// There needs to be a start rule called 'S' and it can only have one nonterminal as contents
	grammar_t grammar = {
		.length = 7,
		.rules = (rule_t[]){
			{2, (uint32_t[]){ nt('S'), nt('E') }},
			{4, (uint32_t[]){ nt('E'), nt('E'), '+', nt('T') }},
			{2, (uint32_t[]){ nt('E'), nt('T') }},
			{4, (uint32_t[]){ nt('T'), nt('T'), '*', nt('F') }},
			{2, (uint32_t[]){ nt('T'), nt('F') }},
			{4, (uint32_t[]){ nt('F'), '(', nt('E'), ')' }},
			{2, (uint32_t[]){ nt('F'), 'i' }}
		},
		.terminal_count = 6,
		.nonterminal_count = 4,
		.symbols = (uint32_t[]){
			'i', '+', '*', '(', ')', '$',
			nt('S'), nt('E'), nt('T'), nt('F')
		},
		.first_count = 0,
		.first_sets = NULL,
		.follow_count = 0,
		.follow_sets = NULL
	};
	
	build_first_sets(&grammar);
	print_first_sets(&grammar);
	
	build_follow_sets(&grammar);
	print_follow_sets(&grammar);
	
	test_increment_closure(&grammar);
	
	action_p action_table = NULL;
	ssize_t* goto_table = NULL;
	states_t states = grow_lr0_automation(&grammar, &action_table, &goto_table);
	
	
	/*
	lr_action_t action_table[states.length][grammar.terminal_count];
	ssize_t goto_table[states.length][grammar.nonterminal_count];
	// initialize the entire table with -1 (allways 0xffff... in two complement ints)
	memset(goto_table, 0xff, sizeof(goto_table));
	
	for(size_t i = 0; i < states.length; i++) {
		item_set_p state = &states.item_sets[i];
		
		
	}
	*/
	
	return 0;
}

/**

create action table

for every state
	for every item
		var next = item next
		if next is terminal
			action_table[item_index, next] = shift_action( state_of(increment_closure(item, next, grammar)) )
		else if item has no next
			if item head is not start symbol
				for every a in follow(item head)
					action_table[item_index, a] = reduce_action( item_rule_index )
			else
				action_table[item_index, EOF] = accept_action()

 */