#pragma once

#include <stdint.h>
#include <stdlib.h>


typedef struct {
	size_t   code_vaddr;
	uint8_t* code_ptr;
	size_t   code_len;
	size_t   data_vaddr;
	uint8_t* data_ptr;
	size_t   data_len;
} asm_t, *asm_p;

void as_new(asm_p as);
void as_destroy(asm_p as);
void as_clear(asm_p as);


void as_save_code(asm_p as, const char* filename);
void as_save_data(asm_p as, const char* filename);
void as_save_elf(asm_p as, const char* filename);

size_t as_data(asm_p as, const void* ptr, size_t size);

void as_write(asm_p as, const char* format, ...);

typedef struct {
	char* name;
	uint8_t bits;
} asm_var_t, *asm_var_p;
void as_write_with_vars(asm_p as, const char* format, asm_var_t vars[]);

typedef enum {
	ASM_T_REG,
	ASM_T_IMM,
	ASM_T_OP,
	ASM_T_MEM_DISP,
	ASM_T_MEM_REL_DISP,
	ASM_T_MEM_REG_DISP,
} asm_arg_type_t;

typedef struct {
	asm_arg_type_t type;
	uint8_t bits;
	union {
		uint8_t reg;
		uint64_t imm;
		uint8_t op_code;
		struct {
			// [reg + disp]
			uint8_t mem_reg;
			int32_t mem_disp;
		};
	};
} asm_arg_t, asm_arg_p;

static inline asm_arg_t reg(uint8_t index)  { return (asm_arg_t){ ASM_T_REG, .reg = index, .bits = 64 }; }
static inline asm_arg_t regb(uint8_t index) { return (asm_arg_t){ ASM_T_REG, .reg = index, .bits = 8 }; }
static inline asm_arg_t regx(uint8_t index, uint8_t bits) { return (asm_arg_t){ ASM_T_REG, .reg = index, .bits = bits }; }
static inline asm_arg_t imm(uint64_t value) { return (asm_arg_t){ ASM_T_IMM, .imm = value }; }
static inline asm_arg_t op(uint8_t op_code) { return (asm_arg_t){ ASM_T_OP, .op_code = op_code }; }
static inline asm_arg_t memd(int32_t disp)  { return (asm_arg_t){ ASM_T_MEM_DISP, .mem_disp = disp, .bits = 64 }; }
static inline asm_arg_t memdb(int32_t disp) { return (asm_arg_t){ ASM_T_MEM_DISP, .mem_disp = disp, .bits = 8 }; }
static inline asm_arg_t reld(int32_t disp)  { return (asm_arg_t){ ASM_T_MEM_REL_DISP, .mem_disp = disp, .bits = 64 }; }
static inline asm_arg_t reldb(int32_t disp) { return (asm_arg_t){ ASM_T_MEM_REL_DISP, .mem_disp = disp, .bits = 8 }; }
static inline asm_arg_t memrd(asm_arg_t reg, int32_t disp) {
	asm_arg_t arg = { 0 };
	arg.type = ASM_T_MEM_REG_DISP;
	arg.bits = 64;
	arg.mem_reg = reg.reg;
	arg.mem_disp = disp;
	return arg;
}
static inline asm_arg_t memrdb(asm_arg_t reg, int32_t disp) {
	asm_arg_t arg = { 0 };
	arg.type = ASM_T_MEM_REG_DISP;
	arg.bits = 8;
	arg.mem_reg = reg.reg;
	arg.mem_disp = disp;
	return arg;
}


#define R0  reg(0)
#define R1  reg(1)
#define R2  reg(2)
#define R3  reg(3)
#define R4  reg(4)
#define R5  reg(5)
#define R6  reg(6)
#define R7  reg(7)
#define R8  reg(8)
#define R9  reg(9)
#define R10 reg(10)
#define R11 reg(11)
#define R12 reg(12)
#define R13 reg(13)
#define R14 reg(14)
#define R15 reg(15)

#define RAX R0
#define RCX R1
#define RDX R2
#define RBX R3
#define RSP R4
#define RBP R5
#define RSI R6
#define RDI R7

#define R0b  reg8(0)
#define R1b  reg8(1)
#define R2b  reg8(2)
#define R3b  reg8(3)
#define R4b  reg8(4)
#define R5b  reg8(5)
#define R6b  reg8(6)
#define R7b  reg8(7)
#define R8b  reg8(8)
#define R9b  reg8(9)
#define R10b reg8(10)
#define R11b reg8(11)
#define R12b reg8(12)
#define R13b reg8(13)
#define R14b reg8(14)
#define R15b reg8(15)

#define RAXb R0
#define RCXb R1
#define RDXb R2
#define RBXb R3
#define RSPb R4
#define RBPb R5
#define RSIb R6
#define RDIb R7

// Volume 2C - Instruction Set Reference, p77 (B.1.4.7 Condition Test (tttn) Field)
#define CC_OVERFLOW          0b0000
#define CC_NO_OVERFLOW       0b0001
#define CC_BELOW             0b0010
#define CC_ABOVE_OR_EQUAL    0b0011
#define CC_EQUAL             0b0100
#define CC_NOT_EQUAL         0b0101
#define CC_BELOW_OR_EQUAL    0b0110
#define CC_ABOVE             0b0111
#define CC_SIGN              0b1000
#define CC_NO_SIGN           0b1001
#define CC_PARITY_EVEN       0b1010
#define CC_PARITY_ODD        0b1011
#define CC_LESS              0b1100
#define CC_GREATER_OR_EQUAL  0b1101
#define CC_LESS_OR_EQUAL     0b1110
#define CC_GREATER           0b1111


typedef struct {
	size_t base;
	size_t disp_offset;
} asm_jump_slot_t, *asm_jump_slot_p;

void as_syscall(asm_p as);

// Return code position of the immidiate argument (if one is used as src)
ssize_t as_add(asm_p as, asm_arg_t dest, asm_arg_t src);
ssize_t as_sub(asm_p as, asm_arg_t dest, asm_arg_t src);
void as_mul(asm_p as, asm_arg_t src);
void as_div(asm_p as, asm_arg_t src);

void as_mov(asm_p as, asm_arg_t dest, asm_arg_t src);

void as_cmp(asm_p as, asm_arg_t arg1, asm_arg_t arg2);
void as_set_cc(asm_p as, uint8_t condition_code, asm_arg_t dest);
asm_jump_slot_t as_jmp(asm_p as, asm_arg_t target);
asm_jump_slot_t as_jmp_cc(asm_p as, uint8_t condition_code, int32_t displacement);
void as_mark_jmp_slot_target(asm_p as, asm_jump_slot_t jump_slot);

size_t as_target(asm_p as);
void   as_set_jmp_slot_target(asm_p as, asm_jump_slot_t jump_slot, size_t target);

// Returns the virtual address of the current code positon
size_t as_code_vaddr(asm_p as);

ssize_t as_call(asm_p as, asm_arg_t target);
void as_ret(asm_p as, int16_t stack_size_to_pop);
void as_enter(asm_p as, int16_t stack_size, int8_t level);
void as_leave(asm_p as);

void as_push(asm_p as, asm_arg_t source);
void as_pop(asm_p as, asm_arg_t dest);