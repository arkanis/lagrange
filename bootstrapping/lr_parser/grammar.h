#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

/*
collections:
http://www.mission-base.com/peter/source/
*/
#include "pbl/pbl.h"

typedef PblSet* symbol_set_p;
typedef void(*symbol_set_iterator_t)(uint32_t symbol);

int symbol_cmp(const void* a, const void* b){
	const uint32_t* ap = a;
	const uint32_t* bp = b;
	
	if (*ap == *bp)
		return 0;
	return (*ap - *bp);
}

int symbol_hash(const void* sym){
	const uint32_t* symp = sym;
	return pblHtHashValue((const unsigned char *)symp, sizeof(uint32_t));
}

static symbol_set_p symbol_set_new(){
	symbol_set_p set = pblSetNewHashSet();
	pblSetSetCompareFunction(set, symbol_cmp);
	return set;
}

static void symbol_set_destroy(symbol_set_p set){
	pblSetFree(set);
}

static inline size_t symbol_set_add(symbol_set_p set, uint32_t symbol){
	int ret = pblSetAdd(set, (void*)symbol);
	if (ret > 0)
		return 1;
	return 0;
}

static inline bool symbol_set_contains(symbol_set_p set, uint32_t symbol){
	int ret = pblSetContains(set, (void*)symbol);
	return (ret != 0);
}

static inline bool symbol_set_equals(symbol_set_p a, symbol_set_p b){
	int ret = pblSetEquals(a, b);
	return (ret != 0);
}

static inline size_t symbol_set_append(symbol_set_p a, symbol_set_p b){
	int a_size = pblSetSize(a);
	int new_size = pblSetAddAll(a, b);
	return new_size - a_size;
}

static inline symbol_set_p symbol_set_without(symbol_set_p set, uint32_t symbol){
	symbol_set_p new_set = pblSetClone(set);
	pblSetRemoveElement(new_set, (void*)symbol);
	return new_set;
}

static inline size_t symbol_set_length(symbol_set_p set){
	return pblSetSize(set);
}

static inline void symbol_set_iterate(symbol_set_p set, symbol_set_iterator_t iterator){
	PblIterator* it = pblSetIterator(set);
	while( pblIteratorHasNext(it) ) {
		void* value = pblIteratorNext(it);
		iterator((uint32_t)value);
	}
}





/*
typedef struct {
	size_t length;
	uint32_t* symbols;
} symbol_set_t, *symbol_set_p;

static inline size_t symbol_set_add(symbol_set_p set, uint32_t symbol){
	for(size_t i = 0; i < set->length; i++) {
		if ( set->symbols[i] == symbol )
			return 0;
	}
	
	set->length++;
	set->symbols = realloc(set->symbols, set->length * sizeof(uint32_t));
	set->symbols[set->length - 1] = symbol;
	return 1;
}

static inline bool symbol_set_contains(symbol_set_p set, uint32_t symbol){
	for(size_t i = 0; i < set->length; i++) {
		if ( set->symbols[i] == symbol )
			return true;
	}
	
	return false;
}

static inline bool symbol_set_equals(symbol_set_p a, symbol_set_p b){
	if (a->length != b->length)
		return false;
	
	for(size_t i = 0; i < a->length; i++) {
		if ( ! symbol_set_contains(b, a->symbols[i]) )
			return false;
	}
	
	return true;
}

static inline size_t symbol_set_append(symbol_set_p a, symbol_set_p b){
	size_t appended = 0;
	for(size_t i = 0; i < b->length; i++)
		appended += symbol_set_add(a, b->symbols[i]);
	return appended;
}

static inline symbol_set_t symbol_set_without(symbol_set_p set, uint32_t symbol){
	symbol_set_t new_set;
	
	if ( symbol_set_contains(set, symbol) )
		new_set.length = set->length - 1;
	else
		new_set.length = set->length;
	new_set.symbols = malloc(new_set.length * sizeof(uint32_t));
	
	size_t j = 0;
	for(size_t i = 0; i < set->length; i++){
		if (set->symbols[i] != symbol)
			new_set.symbols[j++] = set->symbols[i];
	}
	
	return new_set;
}
*/



#define T_EPSILON 1

#define nt(val)              (val + 255)
#define nt_to_char(symbol)   (symbol - 255)

static inline bool is_nt(uint32_t symbol) {
	return (symbol >  255) ? true : false;
}

static inline bool is_terminal(uint32_t symbol) {
	return ! is_nt(symbol);
}


// Every terminal is a symbol below 255, all above are non terminals
// Form: A -> B c is [nt('A'), nt('B'), 'c']
typedef struct {
	size_t length;
	uint32_t* symbols;
} rule_t, *rule_p;

typedef struct {
	size_t length;
	rule_p rules;
	size_t terminal_count, nonterminal_count;
	uint32_t* symbols;
	// First sets (one set for each non terminal)
	size_t first_count;
	symbol_set_p* first_sets;
	// Follow sets (one set for each non terminal)
	size_t follow_count;
	symbol_set_p* follow_sets;
} grammar_t, *grammar_p;

typedef struct {
	rule_p rule;
	size_t pos;
} item_t, *item_p;

typedef struct {
	size_t length;
	item_p items;
} item_set_t, *item_set_p;

typedef struct {
	size_t length;
	item_set_p item_sets;
} states_t, *states_p;


// An item is reducable if the pos it at the end of the rule.
// Meaning we can apply do a reduce step with that rule.
static inline bool item_reducable(item_p item){
	return (item->pos == item->rule->length);
}

// Returns the symbol of the item rule that is after the current pos (point).
static inline uint32_t item_next(item_p item){
	if ( !( item->pos < item->rule->length) )
		return -1;
	return item->rule->symbols[item->pos];
}

static inline item_t item_increment(item_p item){
	return (item_t){ .rule = item->rule, .pos = item->pos + 1 };
}

static inline bool item_equals(item_p a, item_p b){
	return (a->pos == b->pos) && (a->rule == b->rule);
}

static inline void item_set_add(item_set_p item_set, item_t item){
	for(size_t i = 0; i < item_set->length; i++) {
		if ( item_equals(&item_set->items[i], &item) )
			return;
	}
	
	item_set->length++;
	item_set->items = realloc(item_set->items, item_set->length * sizeof(item_t));
	item_set->items[item_set->length - 1] = item;
}

static inline bool item_set_contains(item_set_p item_set, item_p item){
	for(size_t i = 0; i < item_set->length; i++) {
		if ( item_equals(&item_set->items[i], item) )
			return true;
	}
	
	return false;
}

static inline bool item_set_equals(item_set_p a, item_set_p b){
	if (a->length != b->length)
		return false;
	
	for(size_t i = 0; i < a->length; i++) {
		if ( ! item_set_contains(b, &a->items[i]) )
			return false;
	}
	
	return true;
}