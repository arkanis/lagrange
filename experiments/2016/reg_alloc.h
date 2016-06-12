#pragma once
#include "asm.h"

typedef struct {
	uint16_t allocated_registers;
	uint8_t allocated_reg_count;
	size_t spilled_reg_count;
	size_t spill_depth;
} ra_t, *ra_p;

void ra_new(ra_p ra);
void ra_destroy(ra_p ra);

typedef struct {
	int8_t reg_index;
	int8_t spill_reg;
	size_t spill_depth;
} raa_t, *raa_p;

#define RA_ANY_REG -1
#define RA_NO_SPILL    -1
#define RA_STACK_SPILL -2

raa_t ra_alloc_reg(ra_p ra, asm_p as, int8_t reg_index);
void  ra_free_reg(ra_p ra, asm_p as, raa_t allocation);
void  ra_ensure(ra_p ra, uint8_t allocated_reg_count, size_t spilled_reg_count);
bool  ra_reg_allocated(ra_p ra, uint8_t reg_index);
raa_t ra_empty();
int8_t ra_find_free_reg(ra_p ra);