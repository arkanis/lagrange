#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <elf.h>

int write_elf(void* instructions, size_t instruction_size, void* data, size_t data_size, uint64_t data_base){
	// The linux ELF loader uses the frist 16 pages for something. Whenever we try to map
	// to these the loader kills the new process. Therefore start the code at the 17th page.
	size_t code_offset = 4096 * 16;
	// GCC does this: leaves a 4 MB gap at the start.
	//size_t code_offset = 4096 * 1024;
	
	Elf64_Ehdr header;
	Elf64_Phdr program_header, data_ph;
	
	header = (Elf64_Ehdr){
		.e_ident = {
			ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_SYSV,
			0, 0, 0, 0, 0, 0, 0, 0
		},
		.e_type = ET_EXEC,
		.e_machine = EM_X86_64,
		.e_version = EV_CURRENT,
		.e_entry = code_offset + sizeof(header) + sizeof(program_header)*2, // ip start address
		.e_phoff = sizeof(header), // program header table file offset
		.e_shoff = 0, // section header table file offset (0 = none)
		.e_flags = 0,
		.e_ehsize = sizeof(header),
		.e_phentsize = sizeof(program_header),
		.e_phnum = 2,
		.e_shentsize = 0,  // size of section header table entry
		.e_shnum = 0,  // number of section header entries
		.e_shstrndx = SHN_UNDEF  // no section name string table
	};
	
	
	program_header = (Elf64_Phdr){
		.p_type = PT_LOAD,
		.p_flags = PF_X | PF_R,
		
		// source
		.p_offset = 0,  // file offset
		.p_filesz = sizeof(header) + sizeof(program_header) + instruction_size,  // size in file
		
		// destination
		.p_vaddr = code_offset,  // destination virtual address
		.p_memsz = sizeof(header) + sizeof(program_header) + instruction_size,  // size in memory (may be larger than size in file)
		
		.p_align = 4096,
		.p_paddr = 0  // unused
	};
	
	data_ph = (Elf64_Phdr){
		.p_type = PT_LOAD,
		.p_flags = PF_R,
		
		// source
		.p_offset = 4096,  // file offset
		.p_filesz = data_size,  // size in file
		
		// destination
		.p_vaddr = data_base,  // destination virtual address
		.p_memsz = data_size,  // size in memory (may be larger than size in file)
		
		.p_align = 4096,
		.p_paddr = 0  // unused
	};
	
	size_t file_offset = 0;
	FILE* f = fopen("asm_binary.elf", "wb");
	file_offset += fwrite(&header, 1, sizeof(header), f);
	file_offset += fwrite(&program_header, 1, sizeof(program_header), f);
	file_offset += fwrite(&data_ph, 1, sizeof(data_ph), f);
	file_offset += fwrite(instructions, 1, instruction_size, f);
	
	uint8_t padding = 0;
	while (file_offset < 4096)
		file_offset += fwrite(&padding, 1, 1, f);
	
	file_offset += fwrite(data, 1, data_size, f);
	
	fclose(f);
}

typedef int (*call_t)();

size_t offset = 0;
uint8_t* buffer = NULL;


typedef enum {
	RAX = 0b0000,
	RCX = 0b0001,
	RDX = 0b0010,
	RBX = 0b0011,
	RSP = 0b0100,
	RBP = 0b0101,
	RSI = 0b0110,
	RDI = 0b0111,
	R8 = 0b1000,
	R9 = 0b1001,
	R10 = 0b1010,
	R11 = 0b1011,
	R12 = 0b1100,
	R13 = 0b1101,
	R14 = 0b1110,
	R15 = 0b1111
} reg_t;

#define bit(n, val) ((val >> n) & 1)

uint8_t rex(bool w, bool r, bool b){
	// REX: 0100 WR0B 
	return 0b01000000 | (w << 3) | (r << 2) | b;
}

void mov_r64_r64(reg_t target_reg, reg_t source_reg){
	// register2 to register1
	// 0100 0R0B : 1000 101w : 11 reg1 reg2
	buffer[offset++] = rex( 1, bit(3, target_reg), bit(3, source_reg) );
	buffer[offset++] = 0b10001011;  // opcode
	buffer[offset++] = 0b11000000 | ( (target_reg & 0b00000111) << 3 ) | (source_reg & 0b00000111);
}

void mov_m64_r64(reg_t target_reg, reg_t addr_reg, uint32_t displ){
	// REX
	buffer[offset++] = rex( 1, bit(3, target_reg), bit(3, addr_reg) );
	// Opcode
	buffer[offset++] = 0b10001011;
	// mod reg r/m
	uint8_t address_mode;
	if (displ == 0) {
		address_mode = 0b00000000;  // deref reg
		buffer[offset++] = address_mode | ( (target_reg & 0b00000111) << 3 ) | (addr_reg & 0b00000111);
		// no displ
	} else if (displ < 256) {
		address_mode = 0b01000000;  // deref reg + 8 bit displ
		buffer[offset++] = address_mode | ( (target_reg & 0b00000111) << 3 ) | (addr_reg & 0b00000111);
		// 8 bit displ
		buffer[offset++] = (uint8_t) displ;
	} else {
		address_mode = 0b10000000;  // deref reg + 32 bit displ
		buffer[offset++] = address_mode | ( (target_reg & 0b00000111) << 3 ) | (addr_reg & 0b00000111);
		*((uint32_t*)(buffer + offset)) = displ;
		offset += sizeof(uint32_t);
	}
}

void mov_r64_imm64(reg_t target_reg, uint64_t immediate){
	// immediate64 to qwordregister (alternate encoding) 
	// 0100 100B 1011 1000 reg : imm64
	// REX.W + B8+ rd io	MOV r64, imm64	OI Valid N.E. Move imm64 to r64.
	buffer[offset++] = 0b01001000 | bit(3, target_reg);
	buffer[offset++] = 0b10111000 | (target_reg & 0b00000111);
	*((uint64_t*)(buffer + offset)) = immediate;
	offset += sizeof(uint64_t);
}

void push_r64(reg_t r){
	if (r >= R8)
		buffer[offset++] = rex(0, 0, bit(3, r));
	buffer[offset++] = 0b01010000 | (r & 0b00000111);
}

void pop_r64(reg_t r){
	if (r >= R8)
		buffer[offset++] = rex(0, 0, bit(3, r));
	buffer[offset++] = 0b01011000 | (r & 0b00000111);
}

void int3(){
	buffer[offset++] = 0b11001100;
}

void ret(){
	buffer[offset++] = 0b11000011;
}

void syscall(){
	buffer[offset++] = 0b00001111;
	buffer[offset++] = 0b00000101;
}

void jmp(int32_t displacement){
	if (displacement <= INT8_MAX && displacement >= INT8_MIN) {
		// displacement fits into 8 bits
		buffer[offset++] = 0xEB;
		buffer[offset++] = (int8_t)displacement;
	} else {
		// Need full 32 bit displacement
		buffer[offset++] = 0xE9;
		*((int32_t*)(buffer + offset)) = displacement;
		offset += sizeof(int32_t);
	}
}

void hex(uint8_t byte){
	buffer[offset++] = byte;
}


int main(){
	size_t page_size = 4096;
	uint8_t* page = mmap(NULL, page_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	assert(page != NULL);
	buffer = page;
	
	uint64_t data_base = 4096 * 32;
	char text[] = "Hello World!\nNext line :)\n";
	size_t text_size = sizeof(text);
	
	/*
	// return 7
	push_r64(RBP);
	mov_r64_r64(RBP, RSP);
	
	mov_r64_imm64(RAX, 7);
	
	pop_r64(RBP);
	ret();
	*/
	/*
	// write string to stdout
	push_r64(RBP);
	mov_r64_r64(RBP, RSP);
	
	mov_r64_imm64(RAX, 1);  // write
	mov_r64_imm64(RDI, 1); // stdout
	mov_r64_imm64(RSI, (uint64_t)text);  // buffer ptr
	mov_r64_imm64(RDX, text_size);  // size
	syscall();
	
	pop_r64(RBP);
	ret();
	*/
	/*
	mov_r64_r64(R13, RDI);
	mov_m64_r64(R15, RAX, 512);
	push_r64(R15);
	pop_r64(R9);
	int3();
	ret();
	mov_r64_imm64(R15, 8);
	syscall();
	*/
	/*
	pop_r64(R10);  // argc
	pop_r64(R11);  // argv
	pop_r64(R12);  // env_p
	*/
	/*
	int3();
	mov_r64_imm64(RAX, 60);  // exit
	mov_r64_imm64(RDI, 7);  // exit code
	syscall();
	*/
	
	//jmp(0x2a);
	mov_r64_imm64(RAX, 1);  // write
	mov_r64_imm64(RDI, 1); // stdout
	mov_r64_imm64(RSI, data_base);  // buffer ptr
	mov_r64_imm64(RDX, text_size);  // size
	syscall();
	
	mov_r64_imm64(RAX, 60);  // exit
	mov_r64_imm64(RDI, 0);  // exit code
	syscall();
	
	FILE* f = fopen("asm_instructions.raw", "wb");
	fwrite(page, offset, 1, f);
	fclose(f);
	
	write_elf(page, offset, text, text_size + 1, data_base);
	
	/*
	call_t func = (call_t) page;
	int x = func();
	printf("x: %d\n", x);
	*/
	munmap(page, page_size);
	return 0;
}