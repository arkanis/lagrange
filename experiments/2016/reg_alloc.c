#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "reg_alloc.h"

void ra_new(ra_p ra) {
	ra->allocated_registers = 0;
}

void ra_destroy(ra_p ra) {
	ra->allocated_registers = 0;
}


static inline void mark_reg(ra_p ra, uint8_t reg_index) {
	ra->allocated_registers |= (1 << reg_index);
}

static inline void unmark_reg(ra_p ra, uint8_t reg_index) {
	ra->allocated_registers &= ~(1 << reg_index);
}

static inline bool reg_allocated(ra_p ra, uint8_t reg_index) {
	return ((1 << reg_index) & ra->allocated_registers) != 0;
}

int8_t find_free_reg(ra_p ra) {
	for(int8_t i = 15; i > 0; i--) {
		if ( !reg_allocated(ra, i) )
			return i;
	}
	
	fprintf(stderr, "find_free_reg(): no free register left!\n");
	abort();
	return -1;
}


raa_t ra_alloc_reg(ra_p ra, asm_p as, int8_t reg_index) {
	if (reg_index == -1)
		reg_index = find_free_reg(ra);
	
	raa_t allocation = (raa_t){ reg_index, -1 };
	if ( reg_allocated(ra, reg_index) ) {
		// Spill it
		allocation.spill_reg = find_free_reg(ra);
		as_mov(as, reg(allocation.spill_reg), reg(reg_index));
		
		// Mark the spill register as allocated
		mark_reg(ra, allocation.spill_reg);
	} else {
		// No need to spill, but mark the requested reg as allocated since it
		// isn't yet
		mark_reg(ra, reg_index);
	}
	
	return allocation;
}

void ra_free_reg(ra_p ra, asm_p as, raa_t allocation) {
	if (allocation.spill_reg != -1) {
		as_mov(as, reg(allocation.reg_index), reg(allocation.spill_reg));
		unmark_reg(ra, allocation.spill_reg);
	} else if (allocation.reg_index != -1) {
		unmark_reg(ra, allocation.reg_index);
	}
}


/*
int main() {
	ra_p ra = &(ra_t){ 0 };
	ra_new(ra);
	asm_p as = &(asm_t){ 0 };
	as_new(as);
	
	raa_t a1 = ra_alloc_reg(ra, as, RAX.reg);
		raa_t a2 = ra_alloc_reg(ra, as, RAX.reg);
			raa_t a3 = ra_alloc_reg(ra, as, RAX.reg);
			raa_t a31 = ra_alloc_reg(ra, as, RAX.reg);
			raa_t a32 = ra_alloc_reg(ra, as, RDI.reg);
			raa_t a33 = ra_alloc_reg(ra, as, RSI.reg);
			raa_t a34 = ra_alloc_reg(ra, as, RDX.reg);
			raa_t a35 = ra_alloc_reg(ra, as, RCX.reg);
			raa_t a36 = ra_alloc_reg(ra, as, R8.reg);
			raa_t a37 = ra_alloc_reg(ra, as, R9.reg);
			raa_t a38 = ra_alloc_reg(ra, as, R10.reg);
			raa_t a39 = ra_alloc_reg(ra, as, R11.reg);
			
			
			ra_free_reg(ra, as, a39);
			ra_free_reg(ra, as, a38);
			ra_free_reg(ra, as, a37);
			ra_free_reg(ra, as, a36);
			ra_free_reg(ra, as, a35);
			ra_free_reg(ra, as, a34);
			ra_free_reg(ra, as, a33);
			ra_free_reg(ra, as, a32);
			ra_free_reg(ra, as, a31);
			ra_free_reg(ra, as, a3);
		ra_free_reg(ra, as, a2);
	ra_free_reg(ra, as, a1);
	
	as_save_code(as, "ra_instructions.raw");
	as_destroy(as);
	ra_destroy(ra);
	return 0;
}
*/