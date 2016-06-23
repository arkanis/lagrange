#include <stdbool.h>
#include <stdio.h>
#include "disassembler_utils.h"
#include "../asm.h"

#define SLIM_TEST_IMPLEMENTATION
#include "slim_test.h"


//
// Helper functions
//

bool code_cmp(asm_p as, uint8_t* data_ptr, size_t data_len) {
	if (as->code_len != data_len)
		return false;
	
	for(size_t i = 0; i < data_len; i++) {
		if (as->code_ptr[i] != data_ptr[i])
			return false;
	}
	
	return true;
}


//
// Test cases
//

void test_empty_and_free() {
	asm_t as1 = as_empty();
	as_free(&as1);
	
	asm_p as2 = &(asm_t){ 0 };
	as_free(as2);
}

void test_write() {
	asm_p as = &(asm_t){ 0 };
	
	as_write(as, "0000 1111");
	st_check( code_cmp(as, (uint8_t[]){ 0x0f }, 1) );
	as_free(as);
	
	as_write(as, "0000 1111 : 1000 1111");
	st_check( code_cmp(as, (uint8_t[]){ 0x0f, 0x8f }, 2) );
	as_free(as);
	
	as_write(as, "0100 WRXB", 0, 1, 0, 1);
	st_check( code_cmp(as, (uint8_t[]){ 0b01000101 }, 1) );
	as_free(as);
	
	as_write(as, "mm rrr bbb", 0b00, 0b111, 0b110);
	st_check( code_cmp(as, (uint8_t[]){ 0b00111110 }, 1) );
	as_free(as);
	
	as_write(as, "xxxx xxxx", 0b10110001);
	st_check( code_cmp(as, (uint8_t[]){ 0b10110001 }, 1) );
	as_free(as);
	
	as_write(as, "0000 1111 : xxxx 1111", 0b1010);
	st_check( code_cmp(as, (uint8_t[]){ 0x0f, 0b10101111 }, 2) );
	as_free(as);
	
	as_write(as, "1000 0001 : %8d", 0xab);
	st_check( code_cmp(as, (uint8_t[]){ 0b10000001, 0xab }, 2) );
	as_free(as);
	
	// Remember, integers are little-endian in memory
	as_write(as, "1000 0001 : %16d", 0xaabb);
	st_check( code_cmp(as, (uint8_t[]){ 0b10000001, 0xbb, 0xaa }, 3) );
	as_free(as);
	
	as_write(as, "1000 0001 : %32d", 0xaabbccdd);
	st_check( code_cmp(as, (uint8_t[]){ 0b10000001, 0xdd, 0xcc, 0xbb, 0xaa }, 5) );
	as_free(as);
	
	as_write(as, "1000 0001 : %64d", 0xaabbccdd11223344);
	st_check( code_cmp(as, (uint8_t[]){ 0b10000001, 0x44, 0x33, 0x22, 0x11, 0xdd, 0xcc, 0xbb, 0xaa }, 9) );
	as_free(as);
}

void test_write_with_vars() {
	asm_p as = &(asm_t){ 0 };
	
	as_write_with_vars(as, "0010 10dw", (asm_var_t[]){
		{ "d",   1 },
		{ "w",   0 },
		{ "rrr", 0b111 },
		{ NULL, 0 }
	});
	st_check( code_cmp(as, (uint8_t[]){ 0b00101010 }, 1) );
	as_free(as);
	
	as_write_with_vars(as, "mm rrr bbb", (asm_var_t[]){
		{ "mm",  0b10 },
		{ "rrr", 0b110 },
		{ "bbb", 0b001 },
		{ NULL, 0 }
	});
	st_check( code_cmp(as, (uint8_t[]){ 0b10110001 }, 1) );
	as_free(as);
	
	as_write_with_vars(as, "bbb b 00 bb", (asm_var_t[]){
		{ "b",   0b1 },
		{ "bb",  0b10 },
		{ "bbb", 0b001 },
		{ NULL, 0 }
	});
	st_check( code_cmp(as, (uint8_t[]){ 0b00110010 }, 1) );
	as_free(as);
}

void test_write_elf() {
	asm_p as = &(asm_t){ 0 };
	size_t code_vaddr = 4 * 1024 * 1024;
	size_t data_vaddr = 512 * 1024 * 1024;
	
	const char* text_ptr = "Hello World!\n";
	size_t text_len = strlen(text_ptr);
	size_t text_offset = as_data(as, text_ptr, text_len);
	
	// mov RAX, 1   // write syscall
	as_write(as, "0100 000B : 1011 wrrr :  %8d", 0, 0, 0b000, 1);
	// mov RDI, 1   // stdout,             RDI = R7 = 0b0111
	as_write(as, "0100 000B : 1011 wrrr :  %8d", 0, 0, 0b111, 1);
	// mov RSI, text_vaddr   // text_ptr,  RSI = R6 = 0b0110
	as_write(as, "0100 000B : 1011 wrrr : %32d", 0, 1, 0b110, data_vaddr + text_offset);
	// mov RDX, text_len     // text len,  RDX = R2 = 0b0010
	as_write(as, "0100 000B : 1011 wrrr : %32d", 0, 1, 0b010, text_len);
	// syscall
	as_write(as, "0000 1111 : 0000 0101");
	
	// mov RAX, 60  // exit syscall
	as_write(as, "0100 000B : 1011 wrrr :  %8d", 0, 0, 0b000, 60);
	// mov RDI, 1   // exit code,          RDI = R7 = 0b0111
	as_write(as, "0100 000B : 1011 wrrr :  %8d", 0, 0, 0b111, 1);
	// syscall
	as_write(as, "0000 1111 : 0000 0101");
	
	as_save_elf(as, code_vaddr, data_vaddr, "test_write_elf.elf");
	as_free(as);
	
	char* output = NULL;
	int status_code = run_and_delete("test_write_elf.elf", "./test_write_elf.elf", &output);
	
	st_check_int(status_code, 1);
	st_check_str(output, text_ptr);
	
	free(output);
}

/*
void test_basic_instructions() {
	asm_p as = &(asm_t){ 0 };
	char* disassembly = NULL;
	as_new(as);
	
	as_free(as);
		as_syscall(as);
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"syscall \n"
	);
	
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_mov(as, reg(i), imm(0x1122334455667788));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"movabs rax,0x1122334455667788\n"
		"movabs rcx,0x1122334455667788\n"
		"movabs rdx,0x1122334455667788\n"
		"movabs rbx,0x1122334455667788\n"
		"movabs rsp,0x1122334455667788\n"
		"movabs rbp,0x1122334455667788\n"
		"movabs rsi,0x1122334455667788\n"
		"movabs rdi,0x1122334455667788\n"
		"movabs r8,0x1122334455667788\n"
		"movabs r9,0x1122334455667788\n"
		"movabs r10,0x1122334455667788\n"
		"movabs r11,0x1122334455667788\n"
		"movabs r12,0x1122334455667788\n"
		"movabs r13,0x1122334455667788\n"
		"movabs r14,0x1122334455667788\n"
		"movabs r15,0x1122334455667788\n"
	);
	
	free(disassembly);
}

void test_arithmetic_instructions() {
	asm_p as = &(asm_t){ 0 };
	char* disassembly = NULL;
	as_new(as);
	
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_add(as, reg(i), reg(i));
		for(size_t i = 0; i < 16; i++)
			as_add(as, reg(i), imm(0x7abbccdd));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"add    rax,rax\n"
		"add    rcx,rcx\n"
		"add    rdx,rdx\n"
		"add    rbx,rbx\n"
		"add    rsp,rsp\n"
		"add    rbp,rbp\n"
		"add    rsi,rsi\n"
		"add    rdi,rdi\n"
		"add    r8,r8\n"
		"add    r9,r9\n"
		"add    r10,r10\n"
		"add    r11,r11\n"
		"add    r12,r12\n"
		"add    r13,r13\n"
		"add    r14,r14\n"
		"add    r15,r15\n"
		"add    rax,0x7abbccdd\n"
		"add    rcx,0x7abbccdd\n"
		"add    rdx,0x7abbccdd\n"
		"add    rbx,0x7abbccdd\n"
		"add    rsp,0x7abbccdd\n"
		"add    rbp,0x7abbccdd\n"
		"add    rsi,0x7abbccdd\n"
		"add    rdi,0x7abbccdd\n"
		"add    r8,0x7abbccdd\n"
		"add    r9,0x7abbccdd\n"
		"add    r10,0x7abbccdd\n"
		"add    r11,0x7abbccdd\n"
		"add    r12,0x7abbccdd\n"
		"add    r13,0x7abbccdd\n"
		"add    r14,0x7abbccdd\n"
		"add    r15,0x7abbccdd\n"
	);
	
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_sub(as, reg(i), reg(i));
		for(size_t i = 0; i < 16; i++)
			as_sub(as, reg(i), imm(0x7abbccdd));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"sub    rax,rax\n"
		"sub    rcx,rcx\n"
		"sub    rdx,rdx\n"
		"sub    rbx,rbx\n"
		"sub    rsp,rsp\n"
		"sub    rbp,rbp\n"
		"sub    rsi,rsi\n"
		"sub    rdi,rdi\n"
		"sub    r8,r8\n"
		"sub    r9,r9\n"
		"sub    r10,r10\n"
		"sub    r11,r11\n"
		"sub    r12,r12\n"
		"sub    r13,r13\n"
		"sub    r14,r14\n"
		"sub    r15,r15\n"
		"sub    rax,0x7abbccdd\n"
		"sub    rcx,0x7abbccdd\n"
		"sub    rdx,0x7abbccdd\n"
		"sub    rbx,0x7abbccdd\n"
		"sub    rsp,0x7abbccdd\n"
		"sub    rbp,0x7abbccdd\n"
		"sub    rsi,0x7abbccdd\n"
		"sub    rdi,0x7abbccdd\n"
		"sub    r8,0x7abbccdd\n"
		"sub    r9,0x7abbccdd\n"
		"sub    r10,0x7abbccdd\n"
		"sub    r11,0x7abbccdd\n"
		"sub    r12,0x7abbccdd\n"
		"sub    r13,0x7abbccdd\n"
		"sub    r14,0x7abbccdd\n"
		"sub    r15,0x7abbccdd\n"
	);
	
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_mul(as, reg(i));
		for(size_t i = 0; i < 16; i++)
			as_mul(as, memrd(reg(i), 0xbeba));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"mul    rax\n"
		"mul    rcx\n"
		"mul    rdx\n"
		"mul    rbx\n"
		"mul    rsp\n"
		"mul    rbp\n"
		"mul    rsi\n"
		"mul    rdi\n"
		"mul    r8\n"
		"mul    r9\n"
		"mul    r10\n"
		"mul    r11\n"
		"mul    r12\n"
		"mul    r13\n"
		"mul    r14\n"
		"mul    r15\n"
		"mul    QWORD PTR [rax+0xbeba]\n"
		"mul    QWORD PTR [rcx+0xbeba]\n"
		"mul    QWORD PTR [rdx+0xbeba]\n"
		"mul    QWORD PTR [rbx+0xbeba]\n"
		"mul    QWORD PTR [rsp+0xbeba]\n"
		"mul    QWORD PTR [rbp+0xbeba]\n"
		"mul    QWORD PTR [rsi+0xbeba]\n"
		"mul    QWORD PTR [rdi+0xbeba]\n"
		"mul    QWORD PTR [r8+0xbeba]\n"
		"mul    QWORD PTR [r9+0xbeba]\n"
		"mul    QWORD PTR [r10+0xbeba]\n"
		"mul    QWORD PTR [r11+0xbeba]\n"
		"mul    QWORD PTR [r12+0xbeba]\n"
		"mul    QWORD PTR [r13+0xbeba]\n"
		"mul    QWORD PTR [r14+0xbeba]\n"
		"mul    QWORD PTR [r15+0xbeba]\n"
	);
	
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_div(as, reg(i));
		for(size_t i = 0; i < 16; i++)
			as_div(as, memrd(reg(i), 0xbeba));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"div    rax\n"
		"div    rcx\n"
		"div    rdx\n"
		"div    rbx\n"
		"div    rsp\n"
		"div    rbp\n"
		"div    rsi\n"
		"div    rdi\n"
		"div    r8\n"
		"div    r9\n"
		"div    r10\n"
		"div    r11\n"
		"div    r12\n"
		"div    r13\n"
		"div    r14\n"
		"div    r15\n"
		"div    QWORD PTR [rax+0xbeba]\n"
		"div    QWORD PTR [rcx+0xbeba]\n"
		"div    QWORD PTR [rdx+0xbeba]\n"
		"div    QWORD PTR [rbx+0xbeba]\n"
		"div    QWORD PTR [rsp+0xbeba]\n"
		"div    QWORD PTR [rbp+0xbeba]\n"
		"div    QWORD PTR [rsi+0xbeba]\n"
		"div    QWORD PTR [rdi+0xbeba]\n"
		"div    QWORD PTR [r8+0xbeba]\n"
		"div    QWORD PTR [r9+0xbeba]\n"
		"div    QWORD PTR [r10+0xbeba]\n"
		"div    QWORD PTR [r11+0xbeba]\n"
		"div    QWORD PTR [r12+0xbeba]\n"
		"div    QWORD PTR [r13+0xbeba]\n"
		"div    QWORD PTR [r14+0xbeba]\n"
		"div    QWORD PTR [r15+0xbeba]\n"
	);
	
	free(disassembly);
}

void test_addressing_modes() {
	asm_p as = &(asm_t){ 0 };
	char* disassembly = NULL;
	as_new(as);
	
	// reg reg
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_mov(as, reg(i), reg(i));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"mov    rax,rax\n"
		"mov    rcx,rcx\n"
		"mov    rdx,rdx\n"
		"mov    rbx,rbx\n"
		"mov    rsp,rsp\n"
		"mov    rbp,rbp\n"
		"mov    rsi,rsi\n"
		"mov    rdi,rdi\n"
		"mov    r8,r8\n"
		"mov    r9,r9\n"
		"mov    r10,r10\n"
		"mov    r11,r11\n"
		"mov    r12,r12\n"
		"mov    r13,r13\n"
		"mov    r14,r14\n"
		"mov    r15,r15\n"
	);
	
	// reg [RIP + displ]
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_mov(as, reg(i), reld(0xbeba));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"mov    rax,QWORD PTR [rip+0xbeba]        # 0x40bec1\n"
		"mov    rcx,QWORD PTR [rip+0xbeba]        # 0x40bec8\n"
		"mov    rdx,QWORD PTR [rip+0xbeba]        # 0x40becf\n"
		"mov    rbx,QWORD PTR [rip+0xbeba]        # 0x40bed6\n"
		"mov    rsp,QWORD PTR [rip+0xbeba]        # 0x40bedd\n"
		"mov    rbp,QWORD PTR [rip+0xbeba]        # 0x40bee4\n"
		"mov    rsi,QWORD PTR [rip+0xbeba]        # 0x40beeb\n"
		"mov    rdi,QWORD PTR [rip+0xbeba]        # 0x40bef2\n"
		"mov    r8,QWORD PTR [rip+0xbeba]        # 0x40bef9\n"
		"mov    r9,QWORD PTR [rip+0xbeba]        # 0x40bf00\n"
		"mov    r10,QWORD PTR [rip+0xbeba]        # 0x40bf07\n"
		"mov    r11,QWORD PTR [rip+0xbeba]        # 0x40bf0e\n"
		"mov    r12,QWORD PTR [rip+0xbeba]        # 0x40bf15\n"
		"mov    r13,QWORD PTR [rip+0xbeba]        # 0x40bf1c\n"
		"mov    r14,QWORD PTR [rip+0xbeba]        # 0x40bf23\n"
		"mov    r15,QWORD PTR [rip+0xbeba]        # 0x40bf2a\n"
	);
	
	// reg [displ]
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_mov(as, reg(i), memd(0xbeba));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		// Funny syntax when disassembling with intel syntax. Looks normal with
		// GNU syntax.
		"mov    rax,QWORD PTR ds:0xbeba\n"
		"mov    rcx,QWORD PTR ds:0xbeba\n"
		"mov    rdx,QWORD PTR ds:0xbeba\n"
		"mov    rbx,QWORD PTR ds:0xbeba\n"
		"mov    rsp,QWORD PTR ds:0xbeba\n"
		"mov    rbp,QWORD PTR ds:0xbeba\n"
		"mov    rsi,QWORD PTR ds:0xbeba\n"
		"mov    rdi,QWORD PTR ds:0xbeba\n"
		"mov    r8,QWORD PTR ds:0xbeba\n"
		"mov    r9,QWORD PTR ds:0xbeba\n"
		"mov    r10,QWORD PTR ds:0xbeba\n"
		"mov    r11,QWORD PTR ds:0xbeba\n"
		"mov    r12,QWORD PTR ds:0xbeba\n"
		"mov    r13,QWORD PTR ds:0xbeba\n"
		"mov    r14,QWORD PTR ds:0xbeba\n"
		"mov    r15,QWORD PTR ds:0xbeba\n"
	);
	
	// reg [reg + displ]
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_mov(as, reg(i), memrd(reg(i), 0xbeba));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"mov    rax,QWORD PTR [rax+0xbeba]\n"
		"mov    rcx,QWORD PTR [rcx+0xbeba]\n"
		"mov    rdx,QWORD PTR [rdx+0xbeba]\n"
		"mov    rbx,QWORD PTR [rbx+0xbeba]\n"
		"mov    rsp,QWORD PTR [rsp+0xbeba]\n"
		"mov    rbp,QWORD PTR [rbp+0xbeba]\n"
		"mov    rsi,QWORD PTR [rsi+0xbeba]\n"
		"mov    rdi,QWORD PTR [rdi+0xbeba]\n"
		"mov    r8,QWORD PTR [r8+0xbeba]\n"
		"mov    r9,QWORD PTR [r9+0xbeba]\n"
		"mov    r10,QWORD PTR [r10+0xbeba]\n"
		"mov    r11,QWORD PTR [r11+0xbeba]\n"
		"mov    r12,QWORD PTR [r12+0xbeba]\n"
		"mov    r13,QWORD PTR [r13+0xbeba]\n"
		"mov    r14,QWORD PTR [r14+0xbeba]\n"
		"mov    r15,QWORD PTR [r15+0xbeba]\n"
	);
	
	free(disassembly);
}

void test_compare_instructions() {
	asm_p as = &(asm_t){ 0 };
	char* disassembly = NULL;
	as_new(as);
	
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_cmp(as, reg(i), reg(i));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"cmp    rax,rax\n"
		"cmp    rcx,rcx\n"
		"cmp    rdx,rdx\n"
		"cmp    rbx,rbx\n"
		"cmp    rsp,rsp\n"
		"cmp    rbp,rbp\n"
		"cmp    rsi,rsi\n"
		"cmp    rdi,rdi\n"
		"cmp    r8,r8\n"
		"cmp    r9,r9\n"
		"cmp    r10,r10\n"
		"cmp    r11,r11\n"
		"cmp    r12,r12\n"
		"cmp    r13,r13\n"
		"cmp    r14,r14\n"
		"cmp    r15,r15\n"
	);
	
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_cmp(as, reg(i), memrd(reg(i), 0xbeba));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"cmp    rax,QWORD PTR [rax+0xbeba]\n"
		"cmp    rcx,QWORD PTR [rcx+0xbeba]\n"
		"cmp    rdx,QWORD PTR [rdx+0xbeba]\n"
		"cmp    rbx,QWORD PTR [rbx+0xbeba]\n"
		"cmp    rsp,QWORD PTR [rsp+0xbeba]\n"
		"cmp    rbp,QWORD PTR [rbp+0xbeba]\n"
		"cmp    rsi,QWORD PTR [rsi+0xbeba]\n"
		"cmp    rdi,QWORD PTR [rdi+0xbeba]\n"
		"cmp    r8,QWORD PTR [r8+0xbeba]\n"
		"cmp    r9,QWORD PTR [r9+0xbeba]\n"
		"cmp    r10,QWORD PTR [r10+0xbeba]\n"
		"cmp    r11,QWORD PTR [r11+0xbeba]\n"
		"cmp    r12,QWORD PTR [r12+0xbeba]\n"
		"cmp    r13,QWORD PTR [r13+0xbeba]\n"
		"cmp    r14,QWORD PTR [r14+0xbeba]\n"
		"cmp    r15,QWORD PTR [r15+0xbeba]\n"
	);
	
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_cmp(as, reg(i), imm(0x11223344));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"cmp    rax,0x11223344\n"
		"cmp    rcx,0x11223344\n"
		"cmp    rdx,0x11223344\n"
		"cmp    rbx,0x11223344\n"
		"cmp    rsp,0x11223344\n"
		"cmp    rbp,0x11223344\n"
		"cmp    rsi,0x11223344\n"
		"cmp    rdi,0x11223344\n"
		"cmp    r8,0x11223344\n"
		"cmp    r9,0x11223344\n"
		"cmp    r10,0x11223344\n"
		"cmp    r11,0x11223344\n"
		"cmp    r12,0x11223344\n"
		"cmp    r13,0x11223344\n"
		"cmp    r14,0x11223344\n"
		"cmp    r15,0x11223344\n"
	);
	
	free(disassembly);
}

void test_jump_instructions() {
	asm_p as = &(asm_t){ 0 };
	char* disassembly = NULL;
	as_new(as);
	
	as_free(as);
		as_jmp(as, reld(0x11223344));
		for(size_t i = 0; i < 16; i++)
			as_jmp(as, reg(i));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"jmp    0x11623349\n"
		"jmp    rax\n"
		"jmp    rcx\n"
		"jmp    rdx\n"
		"jmp    rbx\n"
		"jmp    rsp\n"
		"jmp    rbp\n"
		"jmp    rsi\n"
		"jmp    rdi\n"
		"jmp    r8\n"
		"jmp    r9\n"
		"jmp    r10\n"
		"jmp    r11\n"
		"jmp    r12\n"
		"jmp    r13\n"
		"jmp    r14\n"
		"jmp    r15\n"
	);
	
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_jmp(as, memrd(reg(i), 0xbeba));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"jmp    QWORD PTR [rax+0xbeba]\n"
		"jmp    QWORD PTR [rcx+0xbeba]\n"
		"jmp    QWORD PTR [rdx+0xbeba]\n"
		"jmp    QWORD PTR [rbx+0xbeba]\n"
		"jmp    QWORD PTR [rsp+0xbeba]\n"
		"jmp    QWORD PTR [rbp+0xbeba]\n"
		"jmp    QWORD PTR [rsi+0xbeba]\n"
		"jmp    QWORD PTR [rdi+0xbeba]\n"
		"jmp    QWORD PTR [r8+0xbeba]\n"
		"jmp    QWORD PTR [r9+0xbeba]\n"
		"jmp    QWORD PTR [r10+0xbeba]\n"
		"jmp    QWORD PTR [r11+0xbeba]\n"
		"jmp    QWORD PTR [r12+0xbeba]\n"
		"jmp    QWORD PTR [r13+0xbeba]\n"
		"jmp    QWORD PTR [r14+0xbeba]\n"
		"jmp    QWORD PTR [r15+0xbeba]\n"
	);
	
	free(disassembly);
}

void test_conditional_instructions() {
	asm_p as = &(asm_t){ 0 };
	char* disassembly = NULL;
	as_new(as);
	
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
	
	as_free(as);
		for(size_t i = 0; i < sizeof(condition_codes) / sizeof(condition_codes[0]); i++)
			as_jmp_cc(as, condition_codes[i], 0x11223344);
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"jo     0x1162334a\n"
		"jno    0x11623350\n"
		"jb     0x11623356\n"
		"jae    0x1162335c\n"
		"je     0x11623362\n"
		"jne    0x11623368\n"
		"jbe    0x1162336e\n"
		"ja     0x11623374\n"
		"js     0x1162337a\n"
		"jns    0x11623380\n"
		"jp     0x11623386\n"
		"jnp    0x1162338c\n"
		"jl     0x11623392\n"
		"jge    0x11623398\n"
		"jle    0x1162339e\n"
		"jg     0x116233a4\n"
	);
	
	as_free(as);
		for(size_t i = 0; i < sizeof(condition_codes) / sizeof(condition_codes[0]); i++)
			as_set_cc(as, condition_codes[i], regb(i));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"seto   al\n"
		"setno  cl\n"
		"setb   dl\n"
		"setae  bl\n"
		"sete   spl\n"
		"setne  bpl\n"
		"setbe  sil\n"
		"seta   dil\n"
		"sets   r8b\n"
		"setns  r9b\n"
		"setp   r10b\n"
		"setnp  r11b\n"
		"setl   r12b\n"
		"setge  r13b\n"
		"setle  r14b\n"
		"setg   r15b\n"
	);
	
	free(disassembly);
}

void test_stack_instructions() {
	asm_p as = &(asm_t){ 0 };
	char* disassembly = NULL;
	as_new(as);
	
	as_free(as);
		as_push(as, imm(0x11223344));
		for(size_t i = 0; i < 16; i++)
			as_push(as, reg(i));
		for(size_t i = 0; i < 16; i++)
			as_push(as, memrd(reg(i), 0xbeba));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"push   0x11223344\n"
		"push   rax\n"
		"push   rcx\n"
		"push   rdx\n"
		"push   rbx\n"
		"push   rsp\n"
		"push   rbp\n"
		"push   rsi\n"
		"push   rdi\n"
		"push   r8\n"
		"push   r9\n"
		"push   r10\n"
		"push   r11\n"
		"push   r12\n"
		"push   r13\n"
		"push   r14\n"
		"push   r15\n"
		"push   QWORD PTR [rax+0xbeba]\n"
		"push   QWORD PTR [rcx+0xbeba]\n"
		"push   QWORD PTR [rdx+0xbeba]\n"
		"push   QWORD PTR [rbx+0xbeba]\n"
		"push   QWORD PTR [rsp+0xbeba]\n"
		"push   QWORD PTR [rbp+0xbeba]\n"
		"push   QWORD PTR [rsi+0xbeba]\n"
		"push   QWORD PTR [rdi+0xbeba]\n"
		"push   QWORD PTR [r8+0xbeba]\n"
		"push   QWORD PTR [r9+0xbeba]\n"
		"push   QWORD PTR [r10+0xbeba]\n"
		"push   QWORD PTR [r11+0xbeba]\n"
		"push   QWORD PTR [r12+0xbeba]\n"
		"push   QWORD PTR [r13+0xbeba]\n"
		"push   QWORD PTR [r14+0xbeba]\n"
		"push   QWORD PTR [r15+0xbeba]\n"
	);
	
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_pop(as, reg(i));
		for(size_t i = 0; i < 16; i++)
			as_pop(as, memrd(reg(i), 0xbeba));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"pop    rax\n"
		"pop    rcx\n"
		"pop    rdx\n"
		"pop    rbx\n"
		"pop    rsp\n"
		"pop    rbp\n"
		"pop    rsi\n"
		"pop    rdi\n"
		"pop    r8\n"
		"pop    r9\n"
		"pop    r10\n"
		"pop    r11\n"
		"pop    r12\n"
		"pop    r13\n"
		"pop    r14\n"
		"pop    r15\n"
		"pop    QWORD PTR [rax+0xbeba]\n"
		"pop    QWORD PTR [rcx+0xbeba]\n"
		"pop    QWORD PTR [rdx+0xbeba]\n"
		"pop    QWORD PTR [rbx+0xbeba]\n"
		"pop    QWORD PTR [rsp+0xbeba]\n"
		"pop    QWORD PTR [rbp+0xbeba]\n"
		"pop    QWORD PTR [rsi+0xbeba]\n"
		"pop    QWORD PTR [rdi+0xbeba]\n"
		"pop    QWORD PTR [r8+0xbeba]\n"
		"pop    QWORD PTR [r9+0xbeba]\n"
		"pop    QWORD PTR [r10+0xbeba]\n"
		"pop    QWORD PTR [r11+0xbeba]\n"
		"pop    QWORD PTR [r12+0xbeba]\n"
		"pop    QWORD PTR [r13+0xbeba]\n"
		"pop    QWORD PTR [r14+0xbeba]\n"
		"pop    QWORD PTR [r15+0xbeba]\n"
	);
	
	free(disassembly);
}

void test_instructions_for_if() {
	asm_p as = &(asm_t){ 0 };
	as_new(as);
	
	asm_jump_slot_t to_false_case, to_end;
	
	as_mov(as, RAX, imm(0));
	as_cmp(as, RAX, imm(0));
	to_false_case = as_jmp_cc(as, CC_NOT_EQUAL, 0);
		as_mov(as, R15, imm(43));
		to_end = as_jmp(as, reld(0));
	as_mark_jmp_slot_target(as, to_false_case);
		as_mov(as, R15, imm(17));
	as_mark_jmp_slot_target(as, to_end);
	as_mov(as, RAX, imm(60));
	as_mov(as, RDI, R15);
	as_syscall(as);
	
	as_save_elf(as, "test_instructions_for_if_true.elf");
	as_free(as);
	
	as_mov(as, RAX, imm(5));
	as_cmp(as, RAX, imm(0));
	to_false_case = as_jmp_cc(as, CC_NOT_EQUAL, 0);
		as_mov(as, R15, imm(43));
		to_end = as_jmp(as, reld(0));
	as_mark_jmp_slot_target(as, to_false_case);
		as_mov(as, R15, imm(17));
	as_mark_jmp_slot_target(as, to_end);
	as_mov(as, RAX, imm(60));
	as_mov(as, RDI, R15);
	as_syscall(as);
	
	as_save_elf(as, "test_instructions_for_if_false.elf");
	as_free(as);
	
	int status_code;
	status_code = run_and_delete("test_instructions_for_if_true.elf", "./test_instructions_for_if_true.elf", NULL);
	st_check_int(status_code, 43);
	status_code = run_and_delete("test_instructions_for_if_false.elf", "./test_instructions_for_if_false.elf", NULL);
	st_check_int(status_code, 17);
}

void test_instructions_for_call() {
	asm_p as = &(asm_t){ 0 };
	char* disassembly = NULL;
	as_new(as);
	
	as_free(as);
		as_call(as, reld(0x7abbccdd));
		as_call(as, RAX);
		as_ret(as, 0);
		as_ret(as, 16);
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"call   0x7afbcce2\n"
		"call   rax\n"
		"ret    \n"
		"ret    0x10\n"
	);
	
	as_free(as);
		as_enter(as, 65, 0);
		as_leave(as);
		as_enter(as, 0, 0);
		as_leave(as);
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"enter  0x41,0x0\n"
		"leave  \n"
		"enter  0x0,0x0\n"
		"leave  \n"
	);
	
	free(disassembly);
}

void test_byte_registers() {
	asm_p as = &(asm_t){ 0 };
	char* disassembly = NULL;
	as_new(as);
	
	// reg reg
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_mov(as, regb(i), regb(i));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"mov    al,al\n"
		"mov    cl,cl\n"
		"mov    dl,dl\n"
		"mov    bl,bl\n"
		"mov    spl,spl\n"
		"mov    bpl,bpl\n"
		"mov    sil,sil\n"
		"mov    dil,dil\n"
		"mov    r8b,r8b\n"
		"mov    r9b,r9b\n"
		"mov    r10b,r10b\n"
		"mov    r11b,r11b\n"
		"mov    r12b,r12b\n"
		"mov    r13b,r13b\n"
		"mov    r14b,r14b\n"
		"mov    r15b,r15b\n"
	);
	
	
	// reg [RIP + displ]
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_mov(as, regb(i), reldb(0x7abbccdd));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"mov    al,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcce4\n"
		"mov    cl,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcceb\n"
		"mov    dl,BYTE PTR [rip+0x7abbccdd]        # 0x7afbccf2\n"
		"mov    bl,BYTE PTR [rip+0x7abbccdd]        # 0x7afbccf9\n"
		"mov    spl,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd00\n"
		"mov    bpl,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd07\n"
		"mov    sil,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd0e\n"
		"mov    dil,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd15\n"
		"mov    r8b,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd1c\n"
		"mov    r9b,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd23\n"
		"mov    r10b,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd2a\n"
		"mov    r11b,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd31\n"
		"mov    r12b,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd38\n"
		"mov    r13b,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd3f\n"
		"mov    r14b,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd46\n"
		"mov    r15b,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd4d\n"
	);
	
	// reg [displ]
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_mov(as, regb(i), memdb(0x7abbccdd));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"mov    al,BYTE PTR ds:0x7abbccdd\n"
		"mov    cl,BYTE PTR ds:0x7abbccdd\n"
		"mov    dl,BYTE PTR ds:0x7abbccdd\n"
		"mov    bl,BYTE PTR ds:0x7abbccdd\n"
		"mov    spl,BYTE PTR ds:0x7abbccdd\n"
		"mov    bpl,BYTE PTR ds:0x7abbccdd\n"
		"mov    sil,BYTE PTR ds:0x7abbccdd\n"
		"mov    dil,BYTE PTR ds:0x7abbccdd\n"
		"mov    r8b,BYTE PTR ds:0x7abbccdd\n"
		"mov    r9b,BYTE PTR ds:0x7abbccdd\n"
		"mov    r10b,BYTE PTR ds:0x7abbccdd\n"
		"mov    r11b,BYTE PTR ds:0x7abbccdd\n"
		"mov    r12b,BYTE PTR ds:0x7abbccdd\n"
		"mov    r13b,BYTE PTR ds:0x7abbccdd\n"
		"mov    r14b,BYTE PTR ds:0x7abbccdd\n"
		"mov    r15b,BYTE PTR ds:0x7abbccdd\n"
	);
	
	// reg [reg + displ]
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_mov(as, regb(i), memrdb(reg(i), 0x7abbccdd));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"mov    al,BYTE PTR [rax+0x7abbccdd]\n"
		"mov    cl,BYTE PTR [rcx+0x7abbccdd]\n"
		"mov    dl,BYTE PTR [rdx+0x7abbccdd]\n"
		"mov    bl,BYTE PTR [rbx+0x7abbccdd]\n"
		"mov    spl,BYTE PTR [rsp+0x7abbccdd]\n"
		"mov    bpl,BYTE PTR [rbp+0x7abbccdd]\n"
		"mov    sil,BYTE PTR [rsi+0x7abbccdd]\n"
		"mov    dil,BYTE PTR [rdi+0x7abbccdd]\n"
		"mov    r8b,BYTE PTR [r8+0x7abbccdd]\n"
		"mov    r9b,BYTE PTR [r9+0x7abbccdd]\n"
		"mov    r10b,BYTE PTR [r10+0x7abbccdd]\n"
		"mov    r11b,BYTE PTR [r11+0x7abbccdd]\n"
		"mov    r12b,BYTE PTR [r12+0x7abbccdd]\n"
		"mov    r13b,BYTE PTR [r13+0x7abbccdd]\n"
		"mov    r14b,BYTE PTR [r14+0x7abbccdd]\n"
		"mov    r15b,BYTE PTR [r15+0x7abbccdd]\n"
	);
	
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_mul(as, regb(i));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"mul    al\n"
		"mul    cl\n"
		"mul    dl\n"
		"mul    bl\n"
		"mul    spl\n"
		"mul    bpl\n"
		"mul    sil\n"
		"mul    dil\n"
		"mul    r8b\n"
		"mul    r9b\n"
		"mul    r10b\n"
		"mul    r11b\n"
		"mul    r12b\n"
		"mul    r13b\n"
		"mul    r14b\n"
		"mul    r15b\n"
	);
	
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_cmp(as, regb(i), regb(i));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"cmp    al,al\n"
		"cmp    cl,cl\n"
		"cmp    dl,dl\n"
		"cmp    bl,bl\n"
		"cmp    spl,spl\n"
		"cmp    bpl,bpl\n"
		"cmp    sil,sil\n"
		"cmp    dil,dil\n"
		"cmp    r8b,r8b\n"
		"cmp    r9b,r9b\n"
		"cmp    r10b,r10b\n"
		"cmp    r11b,r11b\n"
		"cmp    r12b,r12b\n"
		"cmp    r13b,r13b\n"
		"cmp    r14b,r14b\n"
		"cmp    r15b,r15b\n"
	);
	
	as_free(as);
		for(size_t i = 0; i < 16; i++)
			as_cmp(as, regb(i), reldb(0x7abbccdd));
	disassembly = disassemble(as);
	st_check_str(disassembly,
		"cmp    al,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcce4\n"
		"cmp    cl,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcceb\n"
		"cmp    dl,BYTE PTR [rip+0x7abbccdd]        # 0x7afbccf2\n"
		"cmp    bl,BYTE PTR [rip+0x7abbccdd]        # 0x7afbccf9\n"
		"cmp    spl,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd00\n"
		"cmp    bpl,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd07\n"
		"cmp    sil,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd0e\n"
		"cmp    dil,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd15\n"
		"cmp    r8b,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd1c\n"
		"cmp    r9b,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd23\n"
		"cmp    r10b,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd2a\n"
		"cmp    r11b,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd31\n"
		"cmp    r12b,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd38\n"
		"cmp    r13b,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd3f\n"
		"cmp    r14b,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd46\n"
		"cmp    r15b,BYTE PTR [rip+0x7abbccdd]        # 0x7afbcd4d\n"
	);
	
	free(disassembly);
}
*/

int main() {
	st_run(test_empty_and_free);
	st_run(test_write);
	st_run(test_write_with_vars);
	st_run(test_write_elf);
	/*
	st_run(test_basic_instructions);
	st_run(test_arithmetic_instructions);
	st_run(test_addressing_modes);
	st_run(test_compare_instructions);
	st_run(test_jump_instructions);
	st_run(test_conditional_instructions);
	st_run(test_stack_instructions);
	st_run(test_instructions_for_if);
	st_run(test_instructions_for_call);
	st_run(test_byte_registers);
	*/
	return st_show_report();
}