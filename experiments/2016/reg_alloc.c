#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "reg_alloc.h"

void ra_new(ra_p ra) {
	ra->allocated_registers = 0;
	ra->allocated_reg_count = 0;
	ra->spilled_reg_count = 0;
	ra->spill_depth = 0;
}

void ra_destroy(ra_p ra) {
	ra->allocated_registers = 0;
	ra->allocated_reg_count = 0;
	ra->spilled_reg_count = 0;
	ra->spill_depth = 0;
}


static inline void mark_reg(ra_p ra, uint8_t reg_index) {
	ra->allocated_registers |= (1 << reg_index);
}

static inline void unmark_reg(ra_p ra, uint8_t reg_index) {
	ra->allocated_registers &= ~(1 << reg_index);
}

bool ra_reg_allocated(ra_p ra, uint8_t reg_index) {
	return ((1 << reg_index) & ra->allocated_registers) != 0;
}

/**
 * Finds the highest unmarked register but ignores RSP (R4) and RBP (R5) because
 * they're used for addressing of variable access.
 */
int8_t ra_find_free_reg(ra_p ra) {
	for(int8_t i = 15; i > 0; i--) {
		if ( !(ra_reg_allocated(ra, i) && i != RSP.reg && i != RBP.reg) )
			return i;
	}
	
	fprintf(stderr, "ra_find_free_reg(): no free register left!\n");
	abort();
	return -1;
}


raa_t ra_alloc_reg(ra_p ra, asm_p as, int8_t reg_index) {
	if (reg_index == -1)
		reg_index = ra_find_free_reg(ra);
	
	raa_t allocation = (raa_t){ reg_index, RA_NO_SPILL, 0 };
	if ( ra_reg_allocated(ra, reg_index) ) {
		ra->spill_depth++;
		allocation.spill_depth = ra->spill_depth;
		allocation.spill_reg = RA_STACK_SPILL;
		as_push(as, reg(reg_index));
		
		/*
		// Spill it (old code, spilled into free registers, doesn't work for recursive functions)
		allocation.spill_reg = ra_find_free_reg(ra);
		as_mov(as, reg(allocation.spill_reg), reg(reg_index));
		
		// Mark the spill register as allocated
		mark_reg(ra, allocation.spill_reg);
		*/
		
		//printf("ALLOC r%d spill into r%d\n", reg_index, allocation.spill_reg);
		ra->spilled_reg_count++;
	} else {
		// No need to spill, but mark the requested reg as allocated since it
		// isn't yet
		mark_reg(ra, reg_index);
		
		//printf("ALLOC r%d\n", reg_index);
		ra->allocated_reg_count++;
	}
	
	return allocation;
}

void ra_free_reg(ra_p ra, asm_p as, raa_t allocation) {
	if (allocation.spill_reg != RA_NO_SPILL) {
		// We only do RA_STACK_SPILL right now
		if (allocation.spill_reg != RA_STACK_SPILL) {
			fprintf(stderr, "ra_free_reg(): can only handle stack spills for now!\n");
			abort();
		}
		
		if (allocation.spill_depth != ra->spill_depth) {
			fprintf(stderr, "ra_free_reg(): wrong spill depth! Expected %zu, got %zu. Make sure you free regs in the same order as you allocated them.\n",
				ra->spill_depth, allocation.spill_depth);
			abort();
		}
		
		as_pop(as, reg(allocation.reg_index));
		
		/*
		// Old spill restore code from registers, doesn't work for recursive functions
		as_mov(as, reg(allocation.reg_index), reg(allocation.spill_reg));
		unmark_reg(ra, allocation.spill_reg);
		*/
		
		//printf("FREE r%d restore from r%d\n", allocation.reg_index, allocation.spill_reg);
		ra->spilled_reg_count--;
	} else if (allocation.reg_index != -1) {
		unmark_reg(ra, allocation.reg_index);
		
		//printf("FREE r%d\n", allocation.reg_index);
		ra->allocated_reg_count--;
	}
}

void ra_ensure(ra_p ra, uint8_t allocated_reg_count, size_t spilled_reg_count) {
	if (ra->allocated_reg_count != allocated_reg_count || ra->spilled_reg_count != spilled_reg_count) {
		fprintf(stderr, "ra_ensure(): unexpected number of registerd allocated or spilled! "
			"Allocated: %u (%u expected) Spilled: %zu (%zu expected)\n",
			ra->allocated_reg_count, allocated_reg_count,
			ra->spilled_reg_count, spilled_reg_count
		);
		abort();
	}
}

raa_t ra_empty() {
	return (raa_t){ -1, RA_NO_SPILL, 0 };
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