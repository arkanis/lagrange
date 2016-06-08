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

Usually encoded on instruction basis:
- reg      BSWAP RAX
- reg imm  MOV   RAX, 17

Encoded by ModR/M stuff:
- (reg|op) reg               ADD R0, R1
                             DIV RDX → op(110), RDX
                             JMP RDX → op(100), RDX
- (reg|op) memd(addr)        ADD R0, [0xaabbccdd]
                             JMP [0xaabbccdd] → op(100) memd(addr)
- (reg|op) reld(addr)        ADD R0, [RIP + 0xaabbccdd]
                             JMP [RIP + 0xaabbccdd] → op(100) reld(addr)
- (reg|op) memrd(reg, addr)  ADD R0, [R1 + 0xaabbccdd]


- reg       
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

#define WMRM_FIXED_OPERAND_SIZE (1 << 0)
#define WMRM_FORCE_REX          (1 << 1)

bool as_write_modrm(asm_p as, uint32_t flags, const char* format, asm_arg_t dest, asm_arg_t src, asm_var_t vars[]) {
	// Variables for parts of the opcode format:
	// 66H : REX : opcode : mod reg r/m : SIB : disp : imm
	// We use -1 for invalid values
	bool prefix_66h, sib;
	int8_t rex_W, rex_R, rex_X, rex_B;
	int8_t op_d, op_w;
	uint8_t mod, reg, r_m;
	int16_t scale, index, base;
	int32_t* displacement_ptr = NULL;
	
	// operand size   w bit           REX.W bit          66H prefix
	// 8 bit          0 (byte size)   ignored            ignored
	// 16 bit         1 (full size)   0 (default size)   yes (16 bit size)
	// 32 bit         1 (full size)   0 (default size)   no (default size)
	// 64 bit         1 (full size)   1 (64 bit size)    ignored
	
	// Just use 64 bit operand size for everything for now (write no 66H prefix)
	prefix_66h = false;
	op_w = 1;
	// If the operand size is fixed for this instruct (e.g. JMP, SETcc) we don't
	// need to set the REX.W bit.
	rex_W = (flags & WMRM_FIXED_OPERAND_SIZE) ? 0 : 1;
	
	asm_arg_t reg_arg, r_m_arg;
	asm_arg_type_t dt = dest.type, st = src.type;
	if (dt == ASM_T_OP) {
		op_d = -1;
		reg_arg = dest;
		r_m_arg = src;
	} else if (dt == ASM_T_REG) {
		// r/m to register, Table B-11. Encoding of Operation Direction (d) Bit, p78, Volume 2C
		op_d = 1;
		reg_arg = dest;
		r_m_arg = src;
	} else if (st == ASM_T_REG) {
		// register to r/m, Table B-11. Encoding of Operation Direction (d) Bit, p78, Volume 2C
		op_d = 0;
		reg_arg = src;
		r_m_arg = dest;
	} else {
		// Not a ModR/M combination
		return false;
	}
	
	// Handle register argument
	if (reg_arg.type == ASM_T_REG) {
		// ModR/M reg field contains register operand, REX.R bit extends it
		reg = reg_arg.reg;
		rex_R = reg_arg.reg >> 3;
	} else if (reg_arg.type == ASM_T_OP) {
		// ModR/M reg field used as opcode extention, REX.R bit unused, see:
		// 
		// 2.2.1.2 More on REX Prefix Fields, p35: REX.R modifies the ModR/M reg
		// field when that field encodes a GPR, SSE, control or debug register.
		// REX.R is ignored when ModR/M specifies other registers or defines an
		// extended opcode.
		// 
		// Examples:
		//                               |-- USE OF REX.R PROBABLY A DOC MISTAKE!
		// CALL register indirect  0100 WR00 w 1111 1111 : 11  010 reg
		// CALL memory indirect    0100 W0XB w 1111 1111 : mod 010 r/m
		// DIV  Divide RDX:RAX by qwordregister  0100 100B 1111 0111 : 11  110 reg
		// DIV  Divide RDX:RAX by memory64       0100 10XB 1111 0111 : mod 110 r/m
		reg = reg_arg.op_code;
		rex_R = 0;
	} else {
		// Should never happen
		abort();
	}
	
	// Even if the reg operand is an op code extention for the addressing mode
	// it is treated as a register. So we can operate as if the reg argument
	// always is a register.
	switch(r_m_arg.type) {
		case ASM_T_REG:
			// register to register
			mod = 0b11;
			r_m = r_m_arg.reg;
			rex_B = r_m_arg.reg >> 3;
			
			// no SIB byte used
			sib = false;
			rex_X = 0;
			break;
		case ASM_T_MEM_DISP:
			// mod = 00 and R/M = 100 is a special case that signals just a following
			// SIB byte (Volume 2A, p33). Within the SIB byte base = 101 is a special
			// case that when used with mod = 00 signals [scaled index] + disp32. We
			// leave the scaled index empty (index = 100) so we just get the disp32 (p34).
			mod = 0b00;
			r_m = 0b100;
			rex_B = 0;    // r_m field not extended, so REX.B is unused
			
			sib = true;
			base = 0b101;   // no base, [scaled index] + disp32 instead
			index = 0b100;  // no index
			rex_X = 0;      // index doesn't represent a register, so REX.X is not used
			scale = 0b00;   // unused in this case (bits don't matter)
			
			displacement_ptr = &r_m_arg.mem_disp;
			break;
		case ASM_T_MEM_REL_DISP:
			// Volume 2A, p39, 2.2.1.6 RIP-Relative Addressing
			// mod = 00 and R/M = 101 is a special case that signals RIP + disp32
			mod = 0b00;
			r_m = 0b101;
			rex_B = 0;    // r_m field not extended, so REX.B is unused
			
			// no SIB byte used
			sib = false;
			rex_X = 0;
			
			displacement_ptr = &r_m_arg.mem_disp;
			break;
		case ASM_T_MEM_REG_DISP:
			// mod == 10 signals [reg] + disp32 addressing mode, Volume 2A, p33
			mod = 0b10; 
			r_m = r_m_arg.mem_reg & 0b111;
			rex_B = r_m_arg.mem_reg >> 3;
			
			// normally no SIB byte used
			sib = false;
			rex_X = 0;
			
			if (r_m == 0b100) {
				// Would usually mean [RSP] + disp32 but is a special case that signals
				// a following SIB byte. Encode with an SIB byte with no index (special
				// case SS = 00, Index = 100) and RSP as base. In this case REX.B is
				// used to extend SIB.base, so we can just keep the bit in there.
				sib = true;
				scale = 0b00;
				index = 0b100;
				base = r_m_arg.mem_reg;
			}
			
			displacement_ptr = &r_m_arg.mem_disp;
			break;
		default:
			fprintf(stderr, "as_write_modrm(): unsupported memory operand!\n");
			abort();
	}
	
	// Add our local opcode variables to the list of user supplied opcode
	// variables (like condition code, etc.)
	size_t user_supplied_var_count = 0;
	for(size_t i = 0; vars != NULL && vars[i].name != NULL; i++)
		user_supplied_var_count++;
	asm_var_t all_vars[user_supplied_var_count + 2];
	memcpy(all_vars, vars, user_supplied_var_count * sizeof(asm_var_t));
	all_vars[user_supplied_var_count + 0] = (asm_var_t){ "d", op_d };
	all_vars[user_supplied_var_count + 1] = (asm_var_t){ "w", op_w };
	all_vars[user_supplied_var_count + 2] = (asm_var_t){ NULL, 0 };
	
	// Write encoded instruction
	
	// 66H prefix
	if (prefix_66h)
		as_write(as, "0110 0110");
	// REX byte (if necessary). If we want to address some byte registers we
	// need a REX byte even if W, R, X and B bits are set to 0. For now that's
	// what the WMRM_FORCE_REX flag is for.
	if ( (rex_W == 1 || rex_R == 1 || rex_X == 1 || rex_B == 1) || (flags & WMRM_FORCE_REX) )
		as_write(as, "0100 WRXB", rex_W, rex_R, rex_X, rex_B);
	// Opcode byte(s)
	as_write_with_vars(as, format, all_vars);
	// ModR/M byte
	as_write(as, "mm rrr bbb", mod, reg, r_m);
	// SIB byte (if used)
	if (sib)
		as_write(as, "ss xxx bbb", scale, index, base);
	// Displacement (if used)
	if (displacement_ptr != NULL)
		as_write(as, "%32d", *displacement_ptr);
	
	return true;
}


//
// Instructions
//

void as_syscall(asm_p as) {
	// Volume 2C - Instruction Set Reference, p107 (B.2.1 General Purpose Instruction Formats and Encodings for 64-Bit Mode)
	as_write(as, "0000 1111 : 0000 0101");
}


ssize_t as_add(asm_p as, asm_arg_t dest, asm_arg_t src) {
	// Volume 2C - Instruction Set Reference, p90
	if (src.type == ASM_T_IMM) {
		// 1000 00sw : mm 000, but for now we set s = 0 since we only have unsigned regs
		// Presence of REX.W forces sign extention to 64 bits... (Combined Volumes, p1348)
		// Without it it's written into the 32 bit registers which zeros out the upper 32
		// bits of that register. So effectively we can't disable sign-extention in 64 bit
		// mode... :(
		as_write_modrm(as, 0, "1000 000w", op(0b000), dest, NULL);
		as_write(as, "%32d", src.imm);
		return as_target(as) - 4;
	} else if ( as_write_modrm(as, 0, "0000 00dw", dest, src, NULL) ) {
		return -1;
	}
	
	fprintf(stderr, "as_add(): unsupported arg combination!\n");
	abort();
}

ssize_t as_sub(asm_p as, asm_arg_t dest, asm_arg_t src) {
	// Volume 2C - Instruction Set Reference, p106
	if (src.type == ASM_T_IMM) {
		// 1000 00sw : mm 101, but for now we set s = 0 since we only have unsigned regs
		// Presence of REX.W forces sign extention to 64 bits... (Combined Volumes, p1348)
		// Without it it's written into the 32 bit registers which zeros out the upper 32
		// bits of that register. So effectively we can't disable sign-extention in 64 bit
		// mode... :(
		as_write_modrm(as, 0, "1000 000w", op(0b101), dest, NULL);
		as_write(as, "%32d", src.imm);
		return as_target(as) - 4;
	} else if ( as_write_modrm(as, 0, "0010 10dw", dest, src, NULL) ) {
		return -1;
	}
	
	fprintf(stderr, "as_sub(): unsupported arg combination!\n");
	abort();
}

void as_mul(asm_p as, asm_arg_t src) {
	// Volume 2C - Instruction Set Reference, p99
	if ( as_write_modrm(as, 0, "1111 011w", op(0b100), src, NULL) )
		return;
	
	fprintf(stderr, "as_mul(): unsupported arg combination!\n");
	abort();
}

void as_div(asm_p as, asm_arg_t src) {
	// Volume 2C - Instruction Set Reference, p94
	// Divide RDX:RAX by qwordregister  0100 100B : 1111 0111 : 11  110 qwordreg
	// Divide RDX:RAX by memory64       0100 10XB : 1111 0111 : mod 110 r/m
	if ( as_write_modrm(as, 0, "1111 011w", op(0b110), src, NULL) )
		return;
	
	fprintf(stderr, "as_div(): unsupported arg combination!\n");
	abort();
}

void as_mov(asm_p as, asm_arg_t dest, asm_arg_t src) {
	if (dest.type == ASM_T_REG && src.type == ASM_T_IMM) {
		// Volume 2C - Instruction Set Reference, p97 (B.2.1 General Purpose Instruction Formats and Encodings for 64-Bit Mode)
		as_write(as, "0100 100B : 1011 1bbb : %64d", dest.reg >> 3, dest.reg, src.imm);
		return;
	} else if ( as_write_modrm(as, 0, "1000 10dw", dest, src, NULL) ) {
		// memory to reg 0100 0RXB : 1000 101w : mod reg r/m
		// reg to memory 0100 0RXB : 1000 100w : mod reg r/m
		return;
	}
	
	fprintf(stderr, "as_mov(): unsupported arg combination!\n");
	abort();
}


void as_cmp(asm_p as, asm_arg_t arg1, asm_arg_t arg2) {
	// Volume 2C - Instruction Set Reference, p93 (B.2.1 General Purpose Instruction Formats and Encodings for 64-Bit Mode)
	if (arg1.type == ASM_T_REG && arg2.type == ASM_T_IMM) {
		as_write(as, "0100 100B : 1000 0001 : 11 111 bbb : %32d", arg1.reg >> 3, arg1.reg, arg2.imm);
		return;
	} else if ( as_write_modrm(as, 0, "0011 10dw", arg1, arg2, NULL) ) {
		// memory64 with qwordregister  0100 1RXB : 0011 1001  : mod qwordreg r/m
		// qwordregister with memory64  0100 1RXB : 0011 101w1 : mod qwordreg r/m
		//                                                   |-- PROBABLY ERROR IN DOCS
		return;
	} else 
	
	fprintf(stderr, "as_cmp(): unsupported arg combination!\n");
	abort();
}

void as_set_cc(asm_p as, uint8_t condition_code, asm_arg_t dest) {
	// Volume 2C - Instruction Set Reference, p104
	// 
	// Combined Volumes 1, 2ABC, 3ABC, p1308
	// In IA-64 mode, the operand size is fixed at 8 bits. Use of REX prefix enable uniform addressing to additional byte
	// registers. Otherwise, this instruction’s operation is the same as in legacy mode and compatibility mode.
	// 
	// The byte versions of RSI, RDI, RBP and RSP (SIL, DIL, BPL, SPL) can only
	// be encoded by an empty REX byte. We we omit this we get AH, BH, CH and DH.
	// So for now we force the precense of a REX byte with a flag.
	bool result = as_write_modrm(as, WMRM_FIXED_OPERAND_SIZE | WMRM_FORCE_REX, "0000 1111 : 1001 tttt", op(0b000), dest, (asm_var_t[]){
		{ "tttt", condition_code },
		{ NULL, 0 }
	});
	if (result)
		return;
	
	fprintf(stderr, "as_set_cc(): unsupported arg combination!\n");
	abort();
}

asm_jump_slot_t as_jmp(asm_p as, asm_arg_t target) {
	if (target.type == ASM_T_MEM_REL_DISP) {
		as_write(as, "1110 1001 : %32d", target.mem_disp);
		return (asm_jump_slot_t){ .base = as->code_len, .disp_offset = as->code_len - 4 };
	} else if ( as_write_modrm(as, WMRM_FIXED_OPERAND_SIZE, "1111 1111", op(0b100), target, NULL) ) {
		// Combined Volumes 1, 2ABC, 3ABC, p856:
		// In 64-Bit Mode — The instruction’s operation size is fixed at 64 bits. If a selector points to a gate, then RIP equals
		// the 64-bit displacement taken from gate; else RIP equals the zero-extended offset from the far pointer referenced
		// in the instruction.
		// 
		// register indirect  0100 W00B : 1111 1111 : 11  100 reg
		// memory indirect    0100 W0XB : 1111 1111 : mod 100 r/m
		return (asm_jump_slot_t){ .base = as->code_len, .disp_offset = as->code_len - 4 };
	}
	
	fprintf(stderr, "as_jmp(): unsupported arg type!\n");
	abort();
}

asm_jump_slot_t as_jmp_cc(asm_p as, uint8_t condition_code, int32_t displacement) {
	as_write(as, "0000 1111 : 1000 tttt : %32d", condition_code, displacement);
	return (asm_jump_slot_t){ .base = as->code_len, .disp_offset = as->code_len - 4 };
}

void as_mark_jmp_slot_target(asm_p as, asm_jump_slot_t jump_slot) {
	int32_t displacement = as->code_len - (ssize_t)jump_slot.base;
	int32_t* instr_disp_ptr = (int32_t*)(as->code_ptr + jump_slot.disp_offset);
	*instr_disp_ptr = displacement;
}

size_t as_target(asm_p as) {
	return as->code_len;
}

void as_set_jmp_slot_target(asm_p as, asm_jump_slot_t jump_slot, size_t target) {
	int32_t displacement = (ssize_t)target - (ssize_t)jump_slot.base;
	int32_t* instr_disp_ptr = (int32_t*)(as->code_ptr + jump_slot.disp_offset);
	*instr_disp_ptr = displacement;
}


size_t as_code_vaddr(asm_p as) {
	return as->code_vaddr + as->code_len;
}

ssize_t as_call(asm_p as, asm_arg_t target) {
	// Volume 2C - Instruction Set Reference, p93
	if (target.type == ASM_T_MEM_REL_DISP) {
		as_write(as, "1110 1000 : %32d", target.mem_disp);
		return as_target(as) - 4;
	} else if ( as_write_modrm(as, WMRM_FIXED_OPERAND_SIZE, "1111 1111", op(0b010), target, NULL) ) {
		return -1;
	}
	
	fprintf(stderr, "as_call(): unsupported arg type!\n");
	abort();
}

void as_ret(asm_p as, int16_t stack_size_to_pop) {
	// Volume 2C - Instruction Set Reference, p102
	if (stack_size_to_pop == 0) {
		as_write(as, "1100 0011");
	} else {
		as_write(as, "1100 0010 : %16d", stack_size_to_pop);
	}
}

void as_enter(asm_p as, int16_t stack_size, int8_t level) {
	// Volume 2C - Instruction Set Reference, p94
	as_write(as, "1100 1000 : %16d : %8d", stack_size, level);
}

void as_leave(asm_p as) {
	// Volume 2C - Instruction Set Reference, p96
	as_write(as, "1100 1001");
}


void as_push(asm_p as, asm_arg_t source) {
	// Volume 2C - Instruction Set Reference, p100
	if (source.type == ASM_T_IMM) {
		as_write(as, "0110 1000 : %32d", source.imm);
		return;
	} else if ( as_write_modrm(as, WMRM_FIXED_OPERAND_SIZE, "1111 1111", op(0b110), source, NULL) ) {
		return;
	}
	
	fprintf(stderr, "as_push(): unsupported arg type!\n");
	abort();
}

void as_pop(asm_p as, asm_arg_t dest) {
	// Volume 2C - Instruction Set Reference, p100
	if ( as_write_modrm(as, WMRM_FIXED_OPERAND_SIZE, "1000 1111", op(0b000), dest, NULL) )
		return;
	
	fprintf(stderr, "as_pop(): unsupported arg type!\n");
	abort();
}