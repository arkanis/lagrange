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
	/*
	for(size_t i = 0; i < 16; i++)
		as_add(as, reg(i), reg(i));
	for(size_t i = 0; i < 16; i++)
		as_sub(as, reg(i), reg(i));
	for(size_t i = 0; i < 16; i++)
		as_mul(as, reg(i));
	for(size_t i = 0; i < 16; i++)
		as_div(as, reg(i));
	
	for(size_t i = 0; i < 16; i++)
		as_mov(as, reg(i), reg(i));
	for(size_t i = 0; i < 16; i++)
		as_mov(as, reg(i), imm(0x1122334455667788));
	for(size_t i = 0; i < 16; i++)
		as_mov(as, reg(i), reld(0xbeba));
	*/
	for(size_t i = 0; i < 16; i++)
		as_mov(as, reg(i), memd(0xbeba));
	for(size_t i = 0; i < 16; i++)
		as_mov(as, reg(i), memrd(reg(i), 0xbeba));
	
	as_save_code(as, "asm_instructions.raw");
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


int main() {
	st_run(test_write);
	st_run(test_write_with_vars);
	st_run(test_modrm_instructions);
	st_run(test_write_elf);
	return st_show_report();
}