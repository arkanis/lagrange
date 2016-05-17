#pragma once
#include "asm.h"

typedef struct {
	uint16_t allocated_registers;
} ra_t, *ra_p;

void ra_new(ra_p ra);
void ra_destroy(ra_p ra);

typedef struct {
	int8_t reg_index;
	int8_t spill_reg;
} raa_t, *raa_p;

#define RA_ANY_REG -1

raa_t ra_alloc_reg(ra_p ra, asm_p as, int8_t reg_index);
void  ra_free_reg(ra_p ra, asm_p as, raa_t allocation);