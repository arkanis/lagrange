#include <stdbool.h>
#include "../asm.h"

#define SLIM_TEST_IMPLEMENTATION
#include "slim_test.h"


bool code_cmp(asm_p as, uint8_t* data_ptr, size_t data_len) {
	if (as->code_len != data_len)
		return false;
	
	for(size_t i = 0; i < data_len; i++) {
		if (as->code_ptr[i] != data_ptr[i])
			return false;
	}
	
	return true;
}


void test_write() {
	asm_p as = &(asm_t){ 0 };
	as_new(as);
	
	as_write(as, "0000 1111");
	st_check( code_cmp(as, (uint8_t[]){ 0x0f }, 1) );
	as_clear(as);
	
	as_write(as, "0000 1111 : 1000 1111");
	st_check( code_cmp(as, (uint8_t[]){ 0x0f, 0x8f }, 2) );
	as_clear(as);
	
	as_write(as, "0100 WRXB", 0, 1, 0, 1);
	st_check( code_cmp(as, (uint8_t[]){ 0b01000101 }, 1) );
	as_clear(as);
	
	as_write(as, "mm rrr bbb", 0b00, 0b111, 0b110);
	st_check( code_cmp(as, (uint8_t[]){ 0b00111110 }, 1) );
	as_clear(as);
	
	as_write(as, "xxxx xxxx", 0b10110001);
	st_check( code_cmp(as, (uint8_t[]){ 0b10110001 }, 1) );
	as_clear(as);
	
	as_write(as, "0000 1111 : xxxx 1111", 0b1010);
	st_check( code_cmp(as, (uint8_t[]){ 0x0f, 0b10101111 }, 2) );
	as_clear(as);
	
	as_write(as, "1000 0001 : %32d", 0xaabbccdd);
	st_check( code_cmp(as, (uint8_t[]){ 0b10000001, 0xdd, 0xcc, 0xbb, 0xaa }, 5) );
	as_clear(as);
	
	as_destroy(as);
}

void test_write_with_vars() {
	asm_p as = &(asm_t){ 0 };
	as_new(as);
	
	as_write_with_vars(as, "0010 10dw", (asm_var_t[]){
		{ "d",   1 },
		{ "w",   0 },
		{ "rrr", 0b111 },
		{ NULL, 0 }
	});
	st_check( code_cmp(as, (uint8_t[]){ 0b00101010 }, 1) );
	as_clear(as);
	
	as_write_with_vars(as, "mm rrr bbb", (asm_var_t[]){
		{ "mm",  0b10 },
		{ "rrr", 0b110 },
		{ "bbb", 0b001 },
		{ NULL, 0 }
	});
	st_check( code_cmp(as, (uint8_t[]){ 0b10110001 }, 1) );
	as_clear(as);
	
	as_write_with_vars(as, "bbb b 00 bb", (asm_var_t[]){
		{ "b",   0b1 },
		{ "bb",  0b10 },
		{ "bbb", 0b001 },
		{ NULL, 0 }
	});
	st_check( code_cmp(as, (uint8_t[]){ 0b00110010 }, 1) );
	as_clear(as);
	
	as_destroy(as);
}

void test_modrm_instructions() {
	asm_p as = &(asm_t){ 0 };
	as_new(as);
	
	as_syscall(as);
	
	for(size_t i = 0; i < 16; i++)
		as_add(as, reg(i), reg(i));
	for(size_t i = 0; i < 16; i++)
		as_add(as, reg(i), imm(0xaabbccdd));
	for(size_t i = 0; i < 16; i++)
		as_sub(as, reg(i), reg(i));
	for(size_t i = 0; i < 16; i++)
		as_sub(as, reg(i), imm(0xaabbccdd));
	
	for(size_t i = 0; i < 16; i++)
		as_mul(as, reg(i));
	for(size_t i = 0; i < 16; i++)
		as_mul(as, memrd(reg(i), 0xbeba));
	
	for(size_t i = 0; i < 16; i++)
		as_div(as, reg(i));
	for(size_t i = 0; i < 16; i++)
		as_div(as, memrd(reg(i), 0xbeba));
	
	for(size_t i = 0; i < 16; i++)
		as_mov(as, reg(i), reg(i));
	for(size_t i = 0; i < 16; i++)
		as_mov(as, reg(i), imm(0x1122334455667788));
	for(size_t i = 0; i < 16; i++)
		as_mov(as, reg(i), reld(0xbeba));
	
	for(size_t i = 0; i < 16; i++)
		as_mov(as, reg(i), memd(0xbeba));
	for(size_t i = 0; i < 16; i++)
		as_mov(as, reg(i), memrd(reg(i), 0xbeba));
	
	for(size_t i = 0; i < 16; i++)
		as_cmp(as, reg(i), reg(i));
	for(size_t i = 0; i < 16; i++)
		as_cmp(as, reg(i), memrd(reg(i), 0xbeba));
	for(size_t i = 0; i < 16; i++)
		as_cmp(as, reg(i), imm(0x11223344));
	
	
	as_jmp(as, reld(0x11223344));
	for(size_t i = 0; i < 16; i++)
		as_jmp(as, reg(i));
	for(size_t i = 0; i < 16; i++)
		as_jmp(as, memrd(reg(i), 0xbeba));
	
	int condition_codes[] = {
		CC_OVERFLOW, CC_NO_OVERFLOW,
		CC_BELOW, CC_ABOVE_OR_EQUAL,
		CC_EQUAL, CC_NOT_EQUAL,
		CC_BELOW_OR_EQUAL, CC_ABOVE,
		CC_SIGN, CC_NO_SIGN,
		CC_PARITY_EVEN, CC_PARITY_ODD,
		CC_LESS, CC_GREATER_OR_EQUAL,
		CC_LESS_OR_EQUAL, CC_GREATER
	};
	
	for(size_t i = 0; i < sizeof(condition_codes) / sizeof(condition_codes[0]); i++)
		as_jmp_cc(as, condition_codes[i], 0x11223344);
	for(size_t i = 0; i < sizeof(condition_codes) / sizeof(condition_codes[0]); i++)
		as_set_cc(as, condition_codes[i], reg(i));
	
	for(size_t i = 0; i < 16; i++)
		as_push(as, reg(i));
	for(size_t i = 0; i < 16; i++)
		as_push(as, memrd(reg(i), 0xbeba));
	as_push(as, imm(0x11223344));
	for(size_t i = 0; i < 16; i++)
		as_pop(as, reg(i));
	for(size_t i = 0; i < 16; i++)
		as_pop(as, memrd(reg(i), 0xbeba));
	
	as_save_elf(as, "instructions.elf");
	as_destroy(as);
}

void test_write_elf() {
	asm_p as = &(asm_t){ 0 };
	as_new(as);
	
	const char* text_ptr = "Hello World!\n";
	size_t text_len = strlen(text_ptr);
	size_t text_vaddr = as_data(as, text_ptr, text_len);
	
	as_mov(as, RAX, imm(1));  // wirte
	as_mov(as, RDI, imm(1));  // stdout
	as_mov(as, RSI, imm(text_vaddr));  // text ptr
	as_mov(as, RDX, imm(text_len));    // text len
	as_syscall(as);
	
	as_mov(as, RAX, imm(60));  // exit
	as_mov(as, RDI, imm(1));   // exit code
	as_syscall(as);
	
	as_save_elf(as, "asm.elf");
	as_destroy(as);
}

void test_instructions_for_if() {
	asm_p as = &(asm_t){ 0 };
	as_new(as);
	
	as_mov(as, RAX, imm(0));
	as_cmp(as, RAX, imm(0));
	asm_jump_slot_t to_false_case = as_jmp_cc(as, CC_NOT_EQUAL, 0);
		as_mov(as, RAX, imm(60));
		as_mov(as, RDI, imm(1));
		as_syscall(as);
		asm_jump_slot_t to_end = as_jmp(as, reld(0));
	as_mark_jmp_slot_target(as, to_false_case);
		as_mov(as, RAX, imm(60));
		as_mov(as, RDI, imm(0));
		as_syscall(as);
	as_mark_jmp_slot_target(as, to_end);
	as_mov(as, RAX, imm(60));
	as_mov(as, RDI, imm(17));
	as_syscall(as);
	
	as_save_elf(as, "if.elf");
	as_destroy(as);
}

void test_instructions_for_call() {
	asm_p as = &(asm_t){ 0 };
	as_new(as);
	
	as_call(as, reld(16));
	as_call(as, RAX);
	as_ret(as, 0);
	as_ret(as, 16);
	
	as_enter(as, 65, 0);
	as_leave(as);
	as_enter(as, 0, 0);
	as_leave(as);
	
	as_save_elf(as, "call.elf");
	as_destroy(as);
}


int main() {
	st_run(test_write);
	st_run(test_write_with_vars);
	st_run(test_modrm_instructions);
	st_run(test_write_elf);
	st_run(test_instructions_for_if);
	st_run(test_instructions_for_call);
	return st_show_report();
}