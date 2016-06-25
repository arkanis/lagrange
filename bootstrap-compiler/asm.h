#pragma once

#include <stdint.h>
#include <stdlib.h>


/**
 * Struct is designed to be initialized when zeroed out.
 */
typedef struct {
	uint8_t* code_ptr;
	size_t   code_len;
	uint8_t* data_ptr;
	size_t   data_len;
} asm_t, *asm_p;


//
// Basic management functions
//

static inline asm_t as_empty() { return (asm_t){ 0 }; }
              void  as_free(asm_p as);


//
// Output functions
//

/**
 * Virtual addresses that work:
 * code_vaddr: 4 * 1024 * 1024
 * data_vaddr: 512 * 1024 * 1024
 * 
 * The linux ELF loader uses the frist 16 pages for something. Whenever we try
 * to map something to these the loader kills the new process. GCC leaves a 4 MB
 * gap at the start.
 * 
 * Start the data somewhere where the code won't reach it (for now).
 */
void as_save_elf(asm_p as, size_t code_vaddr, size_t data_vaddr, const char* filename);
void as_save_code(asm_p as, const char* filename);
void as_save_data(asm_p as, const char* filename);


//
// Data segment functions
//

/**
 * Appends a memory block to the data segment and returns the byte offset where
 * it starts.
 */
size_t as_data(asm_p as, const void* ptr, size_t size);


//
// printf() like functions for writing bit data into the code segment
//
              
void as_write(asm_p as, const char* format, ...);

typedef struct {
	char* name;
	uint8_t bits;
} asm_var_t, *asm_var_p;
void as_write_with_vars(asm_p as, const char* format, asm_var_t vars[]);


//
// Argument types
//

typedef enum {
	ASM_ARG_REG,
	ASM_ARG_IMM,
	ASM_ARG_DISP,
	ASM_ARG_OP_CODE,
	
	ASM_ARG_MEM_REL_DISP,
	ASM_ARG_MEM_REG,
	ASM_ARG_MEM_DISP,
	ASM_ARG_MEM_REG_DISP,
} asm_arg_type_t;

typedef struct {
	asm_arg_type_t type;
	uint8_t bytes;
	union {
		uint8_t  reg;
		uint64_t imm;
		int32_t  disp;
		uint8_t  op_code;
		struct {
			// address = scale * index + base + disp
			uint8_t base;
			int32_t disp;
			// Implemented when needed:
			//uint8_t scale;
			//uint8_t index;
		} mem;
	};
} asm_arg_t, asm_arg_p;

static inline asm_arg_t as_reg(uint8_t reg_index, uint8_t bytes)        { return (asm_arg_t){ .type = ASM_ARG_REG,     .bytes = bytes, .reg = reg_index }; }
static inline asm_arg_t as_imm(uint64_t value, uint8_t bytes)           { return (asm_arg_t){ .type = ASM_ARG_IMM,     .bytes = bytes, .imm = value     }; }
static inline asm_arg_t as_disp(int32_t displacement)                   { return (asm_arg_t){ .type = ASM_ARG_DISP,    .disp = displacement             }; }
static inline asm_arg_t as_op_code(uint8_t opcode_extention_bits)       { return (asm_arg_t){ .type = ASM_ARG_OP_CODE, .op_code = opcode_extention_bits }; }

// RIP relative memory addressing
static inline asm_arg_t as_mem_rel(int32_t displacement)                { return (asm_arg_t){ .type = ASM_ARG_MEM_REL_DISP, .mem = { .disp = displacement } }; }

// Absolute memory addressing
// All operands fit this scheme, where some parts might be set to 0:
// address = scale * index + base + disp
static inline asm_arg_t as_mem_r(asm_arg_t reg)                         { if(reg.type != ASM_ARG_REG) abort(); return (asm_arg_t){ .type = ASM_ARG_MEM_REG,      .mem = { .base = reg.reg } }; }
static inline asm_arg_t as_mem_d(int32_t displacement)                  {                                      return (asm_arg_t){ .type = ASM_ARG_MEM_DISP,     .mem = { .disp = displacement } }; }
static inline asm_arg_t as_mem_rd(asm_arg_t reg, int32_t displacement)  { if(reg.type != ASM_ARG_REG) abort(); return (asm_arg_t){ .type = ASM_ARG_MEM_REG_DISP, .mem = { .base = reg.reg, .disp = displacement } }; }
// Implemented when needed
//static inline asm_arg_t as_mem_rrd(asm_arg_t index, asm_arg_t base, int32_t displacement);
//static inline asm_arg_t as_mem_srrd(uint8_t scale, asm_arg_t index, asm_arg_t base, int32_t displacement);


//
// Register names
// 
// Planned schema:
//  8 bit = RAXb = R0b
// 16 bit = RAXs = R0s
// 32 bit = EAX  = E0   // Not sure about this yet, maybe RAXe and R0e
// 64 bit = RAX  = R0
//

#define R0  as_reg( 0, 8)
#define R1  as_reg( 1, 8)
#define R2  as_reg( 2, 8)
#define R3  as_reg( 3, 8)
#define R4  as_reg( 4, 8)
#define R5  as_reg( 5, 8)
#define R6  as_reg( 6, 8)
#define R7  as_reg( 7, 8)
#define R8  as_reg( 8, 8)
#define R9  as_reg( 9, 8)
#define R10 as_reg(10, 8)
#define R11 as_reg(11, 8)
#define R12 as_reg(12, 8)
#define R13 as_reg(13, 8)
#define R14 as_reg(14, 8)
#define R15 as_reg(15, 8)

#define RAX R0
#define RCX R1
#define RDX R2
#define RBX R3
#define RSP R4
#define RBP R5
#define RSI R6
#define RDI R7

#define R0b  as_reg( 0, 1)
#define R1b  as_reg( 1, 1)
#define R2b  as_reg( 2, 1)
#define R3b  as_reg( 3, 1)
#define R4b  as_reg( 4, 1)
#define R5b  as_reg( 5, 1)
#define R6b  as_reg( 6, 1)
#define R7b  as_reg( 7, 1)
#define R8b  as_reg( 8, 1)
#define R9b  as_reg( 9, 1)
#define R10b as_reg(10, 1)
#define R11b as_reg(11, 1)
#define R12b as_reg(12, 1)
#define R13b as_reg(13, 1)
#define R14b as_reg(14, 1)
#define R15b as_reg(15, 1)

#define RAXb R0b
#define RCXb R1b
#define RDXb R2b
#define RBXb R3b
#define RSPb R4b
#define RBPb R5b
#define RSIb R6b
#define RDIb R7b


//
// Condition codes
// 
// Volume 2C - Instruction Set Reference, p77 (B.1.4.7 Condition Test (tttn) Field)
//

typedef enum {
	CC_OVERFLOW         = 0b0000,
	CC_NO_OVERFLOW      = 0b0001,
	CC_BELOW            = 0b0010,
	CC_ABOVE_OR_EQUAL   = 0b0011,
	CC_EQUAL            = 0b0100,
	CC_NOT_EQUAL        = 0b0101,
	CC_BELOW_OR_EQUAL   = 0b0110,
	CC_ABOVE            = 0b0111,
	CC_SIGN             = 0b1000,
	CC_NO_SIGN          = 0b1001,
	CC_PARITY_EVEN      = 0b1010,
	CC_PARITY_ODD       = 0b1011,
	CC_LESS             = 0b1100,
	CC_GREATER_OR_EQUAL = 0b1101,
	CC_LESS_OR_EQUAL    = 0b1110,
	CC_GREATER          = 0b1111
} asm_cond_t;


//
// Backpatching support
//

typedef struct {
	uint8_t bytes;  // 0 means no valid backpatch slot
	size_t  value_offset;
	asm_arg_type_t value_type;
	size_t  next_instruction_offset;  // For RIP relative addressing
} asm_slot_t, *asm_slot_p;

size_t as_next_instr_offset(asm_p as);
void   as_patch_slot(asm_p as, asm_slot_t slot, size_t target_offset);


//
// Instructions
//

// Data Transfer Instructions
asm_slot_t as_mov (asm_p as, asm_arg_t dest, asm_arg_t src);
asm_slot_t as_push(asm_p as, asm_arg_t src);
asm_slot_t as_pop (asm_p as, asm_arg_t dest);

// Binary Arithmetic Instructions
asm_slot_t as_add(asm_p as, asm_arg_t dest, asm_arg_t src);
asm_slot_t as_sub(asm_p as, asm_arg_t dest, asm_arg_t src);
asm_slot_t as_mul(asm_p as, asm_arg_t src);
asm_slot_t as_div(asm_p as, asm_arg_t src);

asm_slot_t as_cmp(asm_p as, asm_arg_t arg1, asm_arg_t arg2);

// Bit and Byte Instructions
asm_slot_t as_set_cc(asm_p as, asm_cond_t condition, asm_arg_t dest);

// Control Transfer Instructions
asm_slot_t as_jmp(asm_p as, asm_arg_t target);
asm_slot_t as_jmp_cc(asm_p as, asm_cond_t condition, asm_arg_t target);

// Function call instructions
asm_slot_t as_call(asm_p as, asm_arg_t target);
void       as_ret(asm_p as, int16_t stack_size_to_pop);

void as_enter(asm_p as, int16_t stack_size, int8_t level);
void as_leave(asm_p as);

// Misc instructions
void as_syscall(asm_p as);