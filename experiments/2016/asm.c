#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <elf.h>
#include "asm.h"


void as_new(asm_p as) {
	// The linux ELF loader uses the frist 16 pages for something. Whenever we
	// try to map something to these the loader kills the new process. GCC
	// leaves a 4 MB gap at the start.
	as->code_vaddr = 4 * 1024 * 1024;
	as->code_ptr = NULL;
	as->code_len = 0;
	
	// Start the data somewhere where the code won't reach it (for now)
	as->data_vaddr = 512 * 1024 * 1024;
	as->data_ptr = NULL;
	as->data_len = 0;
}

void as_destroy(asm_p as) {
	free(as->code_ptr);
	free(as->data_ptr);
}

void as_clear(asm_p as) {
	as->code_len = 0;
	as->code_ptr = realloc(as->code_ptr, as->code_len);
	as->data_len = 0;
	as->data_ptr = realloc(as->data_ptr, as->data_len);
}

void as_save_code(asm_p as, const char* filename) {
	FILE* f = fopen(filename, "wb");
	if (!f) {
		perror("as_save_code(): fopen()");
		abort();
	}
	fwrite(as->code_ptr, as->code_len, 1, f);
	fclose(f);
}

void as_save_data(asm_p as, const char* filename) {
	FILE* f = fopen(filename, "wb");
	if (!f) {
		perror("as_save_data(): fopen()");
		abort();
	}
	fwrite(as->data_ptr, as->data_len, 1, f);
	fclose(f);
}


//
// Stuff for the data segment
//

size_t as_data(asm_p as, const void* ptr, size_t size) {
	as->data_len += size;
	as->data_ptr = realloc(as->data_ptr, as->data_len);
	memcpy(as->data_ptr + as->data_len - size, ptr, size);
	return as->data_vaddr + as->data_len - size;
}


//
// ELF stuff
//

/**

	| header
	| prog header (code)
	| prog header (data)
	| code
	| data

**/
void as_save_elf(asm_p as, const char* filename) {
	Elf64_Ehdr elf_header = (Elf64_Ehdr){
		.e_ident = {
			ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_SYSV,
			0, 0, 0, 0, 0, 0, 0, 0
		},
		.e_type = ET_EXEC,
		.e_machine = EM_X86_64,
		.e_version = EV_CURRENT,
		.e_entry = as->code_vaddr, // ip start address
		.e_phoff = sizeof(Elf64_Ehdr), // program header table file offset
		.e_shoff = sizeof(Elf64_Ehdr) + 2*sizeof(Elf64_Phdr), // section header table file offset (0 = none)
		.e_flags = 0,
		.e_ehsize = sizeof(Elf64_Ehdr),
		.e_phentsize = sizeof(Elf64_Phdr),
		.e_phnum = 2,
		.e_shentsize = sizeof(Elf64_Shdr),  // size of section header table entry
		.e_shnum = 4,  // number of section header entries
		.e_shstrndx = 3  // no section name string table
	};
	
	Elf64_Phdr code_prog_header = (Elf64_Phdr){
		.p_type = PT_LOAD,
		.p_flags = PF_X | PF_R,
		
		// source
		.p_offset = 4096,  // file offset
		.p_filesz = as->code_len,  // size in file
		
		// destination
		.p_vaddr = as->code_vaddr,  // destination virtual address
		.p_memsz = as->code_len,  // size in memory (may be larger than size in file)
		
		.p_align = 4096,
		.p_paddr = 0  // unused
	};
	uint64_t code_segment_end = code_prog_header.p_offset + code_prog_header.p_filesz;
	
	Elf64_Phdr data_prog_header = (Elf64_Phdr){
		.p_type = PT_LOAD,
		.p_flags = PF_R,
		
		// source
		.p_offset = code_segment_end + 4096 - (code_segment_end % 4096),  // file offset
		.p_filesz = as->data_len,  // size in file
		
		// destination
		.p_vaddr = as->data_vaddr,  // destination virtual address
		.p_memsz = as->data_len,  // size in memory (may be larger than size in file)
		
		.p_align = 4096,
		.p_paddr = 0  // unused
	};
	uint64_t data_segment_end = data_prog_header.p_offset + data_prog_header.p_filesz;
	
	const char* string_table_ptr = "\0.text\0.data\0.shstrtab\0";
	size_t string_table_size = 23;
	
	Elf64_Shdr code_section_header = (Elf64_Shdr){
		.sh_name = 1,  // Section name (string tbl index)
		.sh_type = SHT_PROGBITS,
		.sh_flags = SHF_ALLOC | SHF_EXECINSTR,
		
		.sh_addr = code_prog_header.p_vaddr,
		.sh_offset = code_prog_header.p_offset,
		.sh_size = code_prog_header.p_filesz,
		
		.sh_link = 0,
		.sh_info = 0,
		.sh_addralign = code_prog_header.p_align,
		.sh_entsize = 0
	};
	Elf64_Shdr data_section_header = (Elf64_Shdr){
		.sh_name = 7,  // Section name (string tbl index)
		.sh_type = SHT_PROGBITS,
		.sh_flags = SHF_ALLOC,
		
		.sh_addr = data_prog_header.p_vaddr,
		.sh_offset = data_prog_header.p_offset,
		.sh_size = data_prog_header.p_filesz,
		
		.sh_link = 0,
		.sh_info = 0,
		.sh_addralign = data_prog_header.p_align,
		.sh_entsize = 0
	};
	Elf64_Shdr strtab_section_header = (Elf64_Shdr){
		.sh_name = 13,  // Section name (string tbl index)
		.sh_type = SHT_STRTAB,
		.sh_flags = SHF_STRINGS,
		
		.sh_addr = 0,
		.sh_offset = data_segment_end + 4096 - (data_segment_end % 4096),
		.sh_size = string_table_size,
		
		.sh_link = 0,
		.sh_info = 0,
		.sh_addralign = data_prog_header.p_align,
		.sh_entsize = 0
	};
	
	FILE* f = fopen(filename, "wb");
	if (!f) {
		perror("as_save_elf(): fopen()");
		abort();
	}
	
	fwrite(&elf_header, 1, sizeof(elf_header), f);
	fwrite(&code_prog_header, 1, sizeof(code_prog_header), f);
	fwrite(&data_prog_header, 1, sizeof(data_prog_header), f);
	
	// Zero out first section header by skipping it, it's unused (index 0 is invalid)
	fseek(f, sizeof(Elf64_Shdr), SEEK_CUR);
	fwrite(&code_section_header, 1, sizeof(code_section_header), f);
	fwrite(&data_section_header, 1, sizeof(data_section_header), f);
	fwrite(&strtab_section_header, 1, sizeof(strtab_section_header), f);
	
	fseek(f, code_prog_header.p_offset, SEEK_SET);
	fwrite(as->code_ptr, 1, as->code_len, f);
	fseek(f, data_prog_header.p_offset, SEEK_SET);
	fwrite(as->data_ptr, 1, as->data_len, f);
	fseek(f, strtab_section_header.sh_offset, SEEK_SET);
	fwrite(string_table_ptr, 1, string_table_size, f);
	
	fclose(f);
}


//
// Binary printf() like helper functions
//

void as_write(asm_p as, const char* format, ...) {
	va_list args;
	va_start(args, format);
	
	uint8_t bits = 0;
	uint8_t bits_used = 0;
	
	for(char const * c = format; *c != '\0'; c++) {
		if (*c == '0') {
			bits <<= 1;
			bits_used++;
		} else if (*c == '1') {
			bits <<= 1;
			bits |= 1;
			bits_used++;
		} else if ( isspace(*c) || *c == ':' ) {
			// Ignore spaces and ':'
		} else if ( isalpha(*c) ) {
			int data = va_arg(args, int);
			uint8_t bits_to_copy = 0;
			for(char letter = *c; *c == letter || isspace(*c); c++) {
				if (*c == letter)
					bits_to_copy++;
			}
			// Go back one char so we can process whatever came after the letter
			// normally on the next loop iteration.
			c--;
			
			for(int bit = bits_to_copy - 1; bit >= 0; bit--) {
				bits <<= 1;
				bits |= (data >> bit) & 1;
				bits_used++;
			}
		} else if (*c == '%') {
			uint8_t width = 0;
			for(c++; isdigit(*c); c++)
				width = width * 10 + (*c - '0');
			
			if (*c != 'd') {
				fprintf(stderr, "as_write(): Unknown type of format specifier: %%%c!\n", *c);
				abort();
			} else if ( !(width == 8 || width == 16 || width == 32 || width == 64) ) {
				fprintf(stderr, "as_write(): Unsupported data width: %hhu!\n", width);
				abort();
			} else if (bits_used != 0) {
				fprintf(stderr, "as_write(): %%d can only be used on byte boundaries!\n");
				abort();
			}
			
			uint64_t data = 0;
			if (width == 64)
				data = va_arg(args, uint64_t);
			else
				data = va_arg(args, uint32_t);
			
			uint8_t bytes = width / 8;
			as->code_len += bytes;
			as->code_ptr = realloc(as->code_ptr, as->code_len);
			memcpy(as->code_ptr + as->code_len - bytes, &data, bytes);
		}
		
		// Push bits into code buffer once the byte is full
		if (bits_used == 8) {
			as->code_len++;
			as->code_ptr = realloc(as->code_ptr, as->code_len);
			as->code_ptr[as->code_len-1] = bits;
			bits = 0;
			bits_used = 0;
		} else if (bits_used > 8) {
			fprintf(stderr, "as_write(): Got more than 8 bits in one go!\n");
			abort();
		}
	}
	
	if (bits_used != 0) {
		fprintf(stderr, "as_write(): Finished not on byte boundary!\n");
		abort();
	}
}

void as_write_with_vars(asm_p as, const char* format, asm_var_t vars[]) {
	uint8_t bits = 0;
	uint8_t bits_used = 0;
	
	for(char const * c = format; *c != '\0'; c++) {
		if (*c == '0') {
			bits <<= 1;
			bits_used++;
		} else if (*c == '1') {
			bits <<= 1;
			bits |= 1;
			bits_used++;
		} else if ( isspace(*c) || *c == ':' ) {
			// Ignore spaces and ':'
		} else if ( isalpha(*c) ) {
			// Got a variable, search for the matching value
			ssize_t found_index = -1;
			size_t found_len = 0;
			for(size_t i = 0; vars[i].name != NULL; i++) {
				size_t var_len = strlen(vars[i].name);
				if ( strncmp(c, vars[i].name, var_len) == 0 && var_len > found_len ) {
					found_index = i;
					found_len = var_len;
				}
			}
			
			if (found_index == -1) {
				fprintf(stderr, "as_write_with_vars(): Failed to find variable at pos %zd\n", c - format);
				abort();
			}
			
			
			uint8_t data = vars[found_index].bits;
			uint8_t bits_to_copy = found_len;
			
			for(int bit = bits_to_copy - 1; bit >= 0; bit--) {
				bits <<= 1;
				bits |= (data >> bit) & 1;
				bits_used++;
			}
			
			// Advance to next char after this variable. Keep in mind that the
			// for loop will increment c at the end (that's what the - 1 is for).
			c += found_len - 1;
		}
		
		// Push bits into code buffer once the byte is full
		if (bits_used == 8) {
			as->code_len++;
			as->code_ptr = realloc(as->code_ptr, as->code_len);
			as->code_ptr[as->code_len-1] = bits;
			bits = 0;
			bits_used = 0;
		} else if (bits_used > 8) {
			fprintf(stderr, "as_write_with_vars(): Got more than 8 bits in one go!\n");
			abort();
		}
	}
	
	if (bits_used != 0) {
		fprintf(stderr, "as_write_with_vars(): Finished not on byte boundary!\n");
		abort();
	}
}


//
// ModRM stuff
//

/**

# Format

	66H : REX : opcode : mod reg r/m : SIB : disp : imm

# Operand size (relevant for all addressing modes)

	encoded via two bits and one prefix:
	
	w      operand size bit, part of opcode
	       w = 0 use 8 bit operand size, w = 1 use full operand size (16, 32 or 64 bits)
	W      64 Bit Operand Size bit
	       W = 0 default operand size (16 or 32 bits), W = 1 64 bit operand size
	66H    prefixing instruction with this byte changes default size to 16 bit
	       set = 16 bit op size, not set = 32 bit (ignored when w or W bit is set)
	
	operand size   w bit           REX.W bit          66H prefix
	8 bit          0 (byte size)   ignored            ignored
	16 bit         1 (full size)   0 (default size)   yes (16 bit size)
	32 bit         1 (full size)   0 (default size)   no (default size)
	64 bit         1 (full size)   1 (64 bit size)    ignored
	
	See Volume 2A - Instruction Set Reference, A-L - 2.2.1.2 More on REX Prefix Fields - page 35

# Argument combinations

- reg       BSWAP RAX
- reg imm   MOV   RAX, 17
- reg reg   ADD   R0, R1
- reg mem(addr)  [0x...]
- reg mem(reg)   [RAX]
- reg mem(rel)   [RIP + rel]
- reg mem(reg, imm)            [reg + imm]
- reg mem(reg, reg, imm)       [reg + reg + imm]
- reg mem(reg, reg, imm, imm)  [reg + reg*imm + imm]


# reg - just one register

	Actually not something for the as_write_modrm() function! Just use as_write() in the
	instruction function directly.

	REX         opcode      ModRM
	0100 W00B : opcode    : 11 opc bbb
	0100 100B : 0000 1111 : 11 001 bbb  (BSWAP - Byte Swap, Volume 2C - Instruction Set Reference, p92)
	
	B    Register bit 3
	bbb  Register bits 2..0

# reg imm - immediate to register

	Again, not something for as_write_modrm().
	
	immediate to register 
	
	REX
	0100 W00B : 1100 011w : 11 000 bbb : imm32   (MOV, immediate32 to qwordregister (zero extend), Volume 2C - Instruction Set Reference, p97)
	0100 100B : 1011 1bbb : imm64   (immediate64 to qwordregister (alternate encoding), bbb was prefixed by 000 but that would make the instruction 3 bits to long. fixed it)
	
	B    Register bit 3
	bbb  Register bits 2..0

# reg reg - register to register

	REX                 ModRM
	0100WR0B : opcode : 11 rrr bbb
	
	Register 1: REX.R + rrr
	Register 2: REX.B + bbb
	
	d  direction bit, might be part of opcode, d = 0 reg1 to reg2, d = 1 reg2 to reg1
	   best always set d = 1, matches intel syntax (dest = src)
	   not marked in instructions, bit 1 in primary opcode byte (the bit left to the w bit)

# reg mem(addr) - direct address

	REX                 ModRM        disp32
	0100WR00 : opcode : 00 rrr 101 : imm32
	
	Would usually express reg mem(EBP). But exception repurposed for this case.
	See Volume 2A - Instruction Set Reference, A-L, p33.

# reg mem(reg) - register indirect

	ModRM
	00 rrr bbb
	
	

// Volume 2C - Instruction Set Reference, p93 (B.2.1 General Purpose Instruction Formats and Encodings for 64-Bit Mode)
as_call(as, addr(0x11223344));
as_call(as, addr_reg(RAX));
as_call(as, addr_mem(0x11223344));





[:reg]
	bbb    bits[0..2] of register _1_ (reg)
	B      bit[3] of register _1_ in REX prefix
	w      operand size bit
	       w = 0 use 8 bit operand size, w = 1 use full operand size (16, 32 or 64 bits)
	W      64 Bit Operand Size bit
	       W = 0 default operand size (16 or 32 bits), W = 1 64 bit operand size
	66H    set to 66H prefix op size should be 16 bit

[:reg, :mem]
	# memory to register 0100 0RXB : 0001 001w : mod reg r/m
	rrr    bits[0..2] of register (reg)
	R      bit[3] of register
	X      ?
	B      ?
	w      operand size bit
	
	mod    address mode
	r/m    address mode
	SIB    scale-index-base byte
	
	[:reg, :mem (:reg) ]               # register indirect
	[:reg, :mem (:reg, :imm) ]         # register indirect + displacement
	[:reg, :mem (:reg, :reg, :imm) ]   # register indirect (base) + register indirect (index) + displacement
	  
**/
bool as_write_modrm(asm_p as, const char* format, asm_arg_t dest, asm_arg_t src) {
	asm_arg_type_t dt = dest.type, st = src.type;
	
	// Just use 64 bit operand size for everything for now (write no 66H prefix)
	uint8_t op_w = 1;
	uint8_t rex_W = 1;
	
	if (dt == ASM_T_REG && st == ASM_T_REG) {
		// REX
		as_write(as, "0100 WR0B", rex_W, dest.reg >> 3, src.reg >> 3);
		as_write_with_vars(as, format, (asm_var_t[]){
			{ "d",  1 },
			{ "w",  op_w },
			{ NULL, 0 }
		});
		// ModRM
		as_write(as, "11 rrr bbb", dest.reg, src.reg);
		return true;
	}
	
	uint8_t direction_bit;
	asm_arg_t reg_arg, mem_arg;
	if (dt == ASM_T_REG) {
		// memory to register
		direction_bit = 1;
		reg_arg = dest;
		mem_arg = src;
	} else {
		// register to memory
		direction_bit = 0;
		reg_arg = src;
		mem_arg = dest;
	}
	
	// Not a ModR/M combination
	if ( ! (reg_arg.type == ASM_T_REG && (mem_arg.type == ASM_T_MEM_DISP || mem_arg.type == ASM_T_MEM_REG_DISP || mem_arg.type == ASM_T_MEM_REL_DISP)) )
		return false;
	
	if (mem_arg.type == ASM_T_MEM_REL_DISP) {
		// Volume 2A, p39, 2.2.1.6 RIP-Relative Addressing
		// mod = 00 and R/M = 101 is a special case that signals RIP + disp32
		uint8_t mod = 0b00;
		uint8_t r_m = 0b101;
		uint8_t reg = reg_arg.reg & 0b111;
		uint8_t rex_R = reg_arg.reg >> 3;
		
		// REX
		as_write(as, "0100 WR00", rex_W, rex_R);
		as_write_with_vars(as, format, (asm_var_t[]){
			{ "d",  direction_bit },
			{ "w",  op_w },
			{ NULL, 0 }
		});
		// ModRM
		as_write(as, "mm rrr bbb", mod, reg, r_m);
		as_write(as, "%32d", mem_arg.mem_disp);
		return true;
	} else if (mem_arg.type == ASM_T_MEM_DISP) {
		// mod = 00 and R/M = 100 is a special case that signals just a following
		// SIB byte (p33). Within the SIB byte base = 101 is a special case that
		// when used with mod = 00 signals [scaled index] + disp32. We leave the
		// scaled index empty (index = 100) so we just get the disp32 (p34).
		uint8_t mod = 0b00;
		uint8_t r_m = 0b100;
		uint8_t reg = reg_arg.reg & 0b111;
		uint8_t rex_R = reg_arg.reg >> 3;
		
		// REX
		as_write(as, "0100 WR00", rex_W, rex_R);
		as_write_with_vars(as, format, (asm_var_t[]){
			{ "d",  direction_bit },
			{ "w",  op_w },
			{ NULL, 0 }
		});
		// ModRM
		as_write(as, "mm rrr bbb", mod, reg, r_m);
		// SIB
		as_write(as, "ss xxx bbb", 0b00, 0b100 /* no index */, 0b101 /* no base, [scaled index] + disp32 instead */);
		as_write(as, "%32d", mem_arg.mem_disp);
		
		return true;
	} else if (mem_arg.type == ASM_T_MEM_REG_DISP) {
		uint8_t mod = 0b10; // [reg] + disp32 addressing mode, Volume 2C, p33
		uint8_t r_m = mem_arg.mem_reg & 0b111;
		uint8_t rex_B = mem_arg.mem_reg >> 3;
		uint8_t reg = reg_arg.reg & 0b111;
		uint8_t rex_R = reg_arg.reg >> 3;
		
		bool write_sib = false;
		uint8_t sib_ss;
		uint8_t sib_index;
		uint8_t sib_base;
		
		if (r_m == 0b100) {
			// Would usually mean [RSP] + disp32 but is a special case that signals
			// a following SIB byte. Encode with an SIB byte with no index (special
			// case SS = 00, Index = 100) and RSP as base. In this case REX.B is
			// used to extend SIB.base, so we can just keep the bit in there.
			write_sib = true;
			sib_ss = 0b00;
			sib_index = 0b100;
			sib_base = mem_arg.mem_reg;
		}
		
		// REX
		as_write(as, "0100 WR0B", rex_W, rex_R, rex_B);
		as_write_with_vars(as, format, (asm_var_t[]){
			{ "d",   direction_bit },
			{ "w",   op_w },
			{ NULL, 0 }
		});
		// ModRM
		as_write(as, "mm rrr bbb", mod, reg, r_m);
		// SIB
		if (write_sib)
			as_write(as, "ss xxx bbb", sib_ss, sib_index, sib_base);
		as_write(as, "%32d", mem_arg.mem_disp);
		
		return true;
	}
	
	return false;
}


//
// Instructions
//

void as_syscall(asm_p as) {
	// Volume 2C - Instruction Set Reference, p107 (B.2.1 General Purpose Instruction Formats and Encodings for 64-Bit Mode)
	/*
	Input:
		RAX ← syscall_no
		RDI
		RSI
		RDX
		RCX
		R8
		R9
	Scratch
		All input regs
		R10
		R11
	Source: http://wiki.osdev.org/Calling_Conventions
	*/
	as_write(as, "0000 1111 : 0000 0101");
}

void as_add(asm_p as, asm_arg_t dest, asm_arg_t src) {
	// Volume 2C - Instruction Set Reference, p90 (B.2.1 General Purpose Instruction Formats and Encodings for 64-Bit Mode)
	if ( as_write_modrm(as, "0000 00dw", dest, src) )
		return;
	
	fprintf(stderr, "as_add(): unsupported arg combination!\n");
	abort();
}

void as_sub(asm_p as, asm_arg_t dest, asm_arg_t src) {
	// Volume 2C - Instruction Set Reference, p106 (B.2.1 General Purpose Instruction Formats and Encodings for 64-Bit Mode)
	if ( as_write_modrm(as, "0010 10dw", dest, src) )
		return;
	
	fprintf(stderr, "as_sub(): unsupported arg combination!\n");
	abort();
}

void as_mul(asm_p as, asm_arg_t src) {
	if (src.type != ASM_T_REG) {
		fprintf(stderr, "as_mul(): src has to be an register for now!\n");
		abort();
	}
	// Volume 2C - Instruction Set Reference, p99 (B.2.1 General Purpose Instruction Formats and Encodings for 64-Bit Mode)
	as_write(as, "0100 W00B : 1111 011w : 11 100 bbb", 1, src.reg >> 3, 1, src.reg);
}

void as_div(asm_p as, asm_arg_t src) {
	if (src.type != ASM_T_REG) {
		fprintf(stderr, "as_div(): src has to be an register for now!\n");
		abort();
	}
	// Volume 2C - Instruction Set Reference, p94 (B.2.1 General Purpose Instruction Formats and Encodings for 64-Bit Mode)
	as_write(as, "0100 W00B : 1111 011w : 11 110 bbb", 1, src.reg >> 3, 1, src.reg);
}

void as_mov(asm_p as, asm_arg_t dest, asm_arg_t src) {
	if ( as_write_modrm(as, "1000 10dw", dest, src) ) {
		// memory to reg 0100 0RXB : 1000 101w : mod reg r/m
		// reg to memory 0100 0RXB : 1000 100w : mod reg r/m
		return;
	} else if (dest.type == ASM_T_REG && src.type == ASM_T_IMM) {
		// Volume 2C - Instruction Set Reference, p97 (B.2.1 General Purpose Instruction Formats and Encodings for 64-Bit Mode)
		as_write(as, "0100 100B : 1011 1bbb : %64d", dest.reg >> 3, dest.reg, src.imm);
		return;
	}
	
	fprintf(stderr, "as_mov(): unsupported arg combination!\n");
	abort();
}