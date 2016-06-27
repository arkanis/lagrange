#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <elf.h>
#include "asm.h"


//
// Basic management functions
//

void as_free(asm_p as) {
	free(as->code_ptr);
	free(as->data_ptr);
	*as = as_empty();
}


//
// Basic save functions
//

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
// ELF functions
//

/**
 * Write the code and data in the assembler buffer into an ELF binary.
 * 
 * Program headers as well as section headers for code and data are written.
 * This way inspection via objdump automatically works.
**/
void as_save_elf(asm_p as, size_t code_vaddr, size_t data_vaddr, const char* filename) {
	Elf64_Ehdr elf_header = (Elf64_Ehdr){
		.e_ident = {
			ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_SYSV,
			0, 0, 0, 0, 0, 0, 0, 0
		},
		.e_type = ET_EXEC,
		.e_machine = EM_X86_64,
		.e_version = EV_CURRENT,
		.e_entry = code_vaddr, // ip start address
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
		.p_vaddr = code_vaddr,  // destination virtual address
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
		.p_vaddr = data_vaddr,  // destination virtual address
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
// Functions to put stuff into the data segment
//

size_t as_data(asm_p as, const void* ptr, size_t size) {
	as->data_len += size;
	as->data_ptr = realloc(as->data_ptr, as->data_len);
	memcpy(as->data_ptr + as->data_len - size, ptr, size);
	return as->data_len - size;
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
// Backpatching support
//

asm_slot_t as_slot_for_last_instr(asm_p as, uint8_t bytes, asm_arg_type_t value_type) {
	return (asm_slot_t) {
		.bytes = bytes,
		.value_offset = as_next_instr_offset(as) - bytes,
		.value_type = value_type,
		.next_instruction_offset = as_next_instr_offset(as)
	};
}

size_t as_next_instr_offset(asm_p as) {
	return as->code_len;
}

void as_patch_slot(asm_p as, asm_slot_t slot, size_t target_offset) {
	if (slot.value_type != ASM_ARG_DISP) {
		fprintf(stderr, "as_patch_slot(): Got a slot that isn't a displacement!\n");
		abort();
	}
	
	int64_t disp_value = (int64_t)target_offset - (int64_t)slot.next_instruction_offset;
	int64_t max = (1LL << (slot.bytes*8 - 1));
	int64_t min = -max - 1;
	if (disp_value < min || disp_value > max) {
		fprintf(stderr, "as_patch_slot(): Calculated displacement is to large for slot!\n");
		abort();
	}
	
	// ATTENTION: Only works for little endian system (like x86)
	memcpy(as->code_ptr + slot.value_offset, &disp_value, slot.bytes);
}


//
// ModRM stuff
//

// The instruction has a defined fixed operand size. So don't add and 66H prefix
// and nither set REX.W or w bits.
#define WMRM_FIXED_OP_SIZE (1 << 0)
// Instruction needs a REX prefix, even if it's empty and unused. For example to
// address 8 bit areas of some registers.
#define WMRM_FORCE_REX     (1 << 1)

bool as_write_modrm(asm_p as, uint32_t flags, const char* format, asm_arg_t dest, asm_arg_t src, asm_slot_p slot, asm_var_t vars[]) {
	// Variables for parts of the opcode format:
	// 66H : REX : opcode : mod reg r/m : SIB : disp : imm
	// The value -1 represents invalid values
	bool prefix_66h, sib;
	int8_t rex_W, rex_R, rex_X, rex_B;
	int8_t op_d, op_w;
	uint8_t mod, reg, r_m;
	int16_t scale, index, base;
	int32_t* displacement_ptr = NULL;
	
	// Map dest and src to the reg and r/m arguments and set the d bit accordingly
	// ASM_ARG_REG
	//   -> can be both
	// ASM_ARG_IMM
	// ASM_ARG_DISP
	//   -> can't be handled by as_write_modrm(), return false
	// ASM_ARG_OP_CODE
	//   -> can on be reg argument, only valid as dest parameter
	// ASM_ARG_MEM_REL_DISP
	// ASM_ARG_MEM_REG
	// ASM_ARG_MEM_DISP
	// ASM_ARG_MEM_REG_DISP
	//   -> can only be reg argument
	
	// Notify caller of invalid arguments or a wrong op_code argument
	asm_arg_type_t dt = dest.type, st = src.type;
	if (dt == ASM_ARG_IMM || dt == ASM_ARG_DISP || st == ASM_ARG_IMM || st == ASM_ARG_DISP) {
		// as_write_modrm() can't handle these argument types
		return false;
	} else if (st == ASM_ARG_OP_CODE) {
		fprintf(stderr, "as_write_modrm(): only dest can be an as_op_code() argument!\n");
		abort();
	}
	
	// Figure out what is the reg and r/m argument based on the dest parameter
	asm_arg_t reg_arg, r_m_arg;
	if (dt == ASM_ARG_OP_CODE) {
		// In the one operand form the reg argument bits are reused as opcode
		// bits. So the op_code argument has to be the reg argument.
		reg_arg = dest;
		r_m_arg = src;
		// No valid direction bit for just one argument
		op_d = -1;
	} else if (dt == ASM_ARG_MEM_REL_DISP || dt == ASM_ARG_MEM_REG || dt == ASM_ARG_MEM_DISP || dt == ASM_ARG_MEM_REG_DISP) {
		// If dest parameter is a memory reference it has to be the r/m argument.
		// In all other cases it can be the reg argument.
		// reg to r/m, reg is source, so d = 0
		// Table B-11. Encoding of Operation Direction (d) Bit, p78, Volume 2C
		op_d = 0;
		reg_arg = src;
		r_m_arg = dest;
	} else {
		// r/m to register, r/m is source, so d = 1
		// Table B-11. Encoding of Operation Direction (d) Bit, p78, Volume 2C
		op_d = 1;
		reg_arg = dest;
		r_m_arg = src;
	}
	
	// Figure out the operand size. If one operand has an undefined size the
	// other one is used. If both operands have a size it has to match.
	// TODO: Add a flag to ignore match, meaning using the max of both. This
	// will be the case for instructions that zero- or sign-extend the source.
	uint8_t bytes = 0;
	if ( dest.bytes == 0 && src.bytes > 0 ) {
		bytes = src.bytes;
	} else if ( src.bytes == 0 && dest.bytes > 0 ) {
		bytes = dest.bytes;
	} else if ( dest.bytes == src.bytes && dest.bytes > 0 ) {
		bytes = dest.bytes;
	} else {
		fprintf(stderr, "as_write_modrm(): dest and src have an unknown or not matching size! dest %d, src %d\n", dest.bytes, src.bytes);
		abort();
	}
	
	// operand size   w bit           REX.W bit          66H prefix
	// 8 bit          0 (byte size)   ignored            ignored
	// 16 bit         1 (full size)   0 (default size)   yes (16 bit size)
	// 32 bit         1 (full size)   0 (default size)   no (default size)
	// 64 bit         1 (full size)   1 (64 bit size)    ignored
	switch(bytes) {
		case 1:
			op_w = 0;
			rex_W = 0;
			prefix_66h = false;
			// The byte versions of RSI, RDI, RBP and RSP (SIL, DIL, BPL, SPL) can only
			// be encoded by an empty REX byte. We we omit this we get AH, BH, CH and DH.
			// So for now we force the precense of a REX byte with a flag.
			// TODO: Only needed for RSI, RDI, RBP and RSP registers, don't set for the rest.
			flags |= WMRM_FORCE_REX;
			break;
		case 2:
			op_w = 1;
			rex_W = 0;
			prefix_66h = true;
			break;
		case 4:
			op_w = 1;
			rex_W = 0;
			prefix_66h = false;
			break;
		case 8:
			op_w = 1;
			rex_W = 1;
			prefix_66h = false;
			break;
		default:
			fprintf(stderr, "as_write_modrm(): unsupported bit sizes of operands!\n");
			abort();
	}
	
	// If the operand size is fixed for this instruct (e.g. JMP, SETcc) we don't
	// need to set the REX.W bit or add the 66H prefix.
	if (flags & WMRM_FIXED_OP_SIZE) {
		op_w = -1;
		rex_W = 0;
		prefix_66h = false;
	}
	
	
	// Handle register argument
	if (reg_arg.type == ASM_ARG_REG) {
		// ModR/M byte reg field contains register operand, REX.R bit extends it
		reg = reg_arg.reg;
		rex_R = reg_arg.reg >> 3;
	} else if (reg_arg.type == ASM_ARG_OP_CODE) {
		// ModR/M byte reg field used as opcode extention, REX.R bit unused, see:
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
		fprintf(stderr, "as_write_modrm(): unsupported reg operand!\n");
		abort();
	}
	
	// Even if the reg operand is an op code extention for the addressing mode
	// it is treated as a register. So we can operate as if the reg argument
	// always is a register.
	switch(r_m_arg.type) {
		case ASM_ARG_REG:
			// register to register
			mod = 0b11;
			r_m = r_m_arg.reg;
			rex_B = r_m_arg.reg >> 3;
			
			// no SIB byte used
			sib = false;
			rex_X = 0;
			break;
		case ASM_ARG_MEM_DISP:
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
			
			displacement_ptr = &r_m_arg.mem.disp;
			break;
		case ASM_ARG_MEM_REL_DISP:
			// Volume 2A, p39, 2.2.1.6 RIP-Relative Addressing
			// mod = 00 and R/M = 101 is a special case that signals RIP + disp32
			mod = 0b00;
			r_m = 0b101;
			rex_B = 0;    // r_m field not extended, so REX.B is unused
			
			// no SIB byte used
			sib = false;
			rex_X = 0;
			
			displacement_ptr = &r_m_arg.mem.disp;
			break;
		case ASM_ARG_MEM_REG_DISP:
			// mod == 10 signals [reg] + disp32 addressing mode, Volume 2A, p33
			mod = 0b10; 
			r_m = r_m_arg.mem.base & 0b111;
			rex_B = r_m_arg.mem.base >> 3;
			
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
				base = r_m_arg.mem.base;
			}
			
			displacement_ptr = &r_m_arg.mem.disp;
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
	if (op_d != -1)
		all_vars[user_supplied_var_count++] = (asm_var_t){ "d", op_d };
	if (op_w != -1)
		all_vars[user_supplied_var_count++] = (asm_var_t){ "w", op_w };
	all_vars[user_supplied_var_count++] = (asm_var_t){ NULL, 0 };
	
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
	if (displacement_ptr != NULL) {
		as_write(as, "%32d", *displacement_ptr);
		if (slot) *slot = as_slot_for_last_instr(as, 4, ASM_ARG_DISP);
	} else {
		if (slot) *slot = as_invalid_slot();
	}
	
	
	return true;
}


//
// Data Transfer Instructions
//

asm_slot_t as_mov(asm_p as, asm_arg_t dest, asm_arg_t src) {
	if (dest.type == ASM_ARG_REG && src.type == ASM_ARG_IMM) {
		// Volume 2C - Instruction Set Reference, p97 (B.2.1 General Purpose Instruction Formats and Encodings for 64-Bit Mode)
		if (dest.bytes != 8) {
			fprintf(stderr, "as_mov(): Only 8 byte immediates supported for now!\n");
			abort();
		}
		as_write(as, "0100 100B : 1011 1bbb : %64d", dest.reg >> 3, dest.reg, src.imm);
		return as_slot_for_last_instr(as, 8, ASM_ARG_IMM);
	} else if ( as_write_modrm(as, 0, "1000 10dw", dest, src, NULL, NULL) ) {
		// memory to reg 0100 0RXB : 1000 101w : mod reg r/m
		// reg to memory 0100 0RXB : 1000 100w : mod reg r/m
		return as_invalid_slot();
	}
	
	fprintf(stderr, "as_mov(): unsupported arg combination!\n");
	abort();
	return as_invalid_slot();
}

asm_slot_t as_push(asm_p as, asm_arg_t src) {
	// Volume 2C - Instruction Set Reference, p100
	if (src.type == ASM_ARG_IMM) {
		switch(src.bytes) {
			case 1:
				as_write(as, "0110 1010 : %8d", src.imm);
				break;
			case 2:
				// PUSH immediate16  0101 0101 : 0110 1000 : imm16
				// 55H (address size) prefix probably error in the docs!
				// 66H (operand size) prefix works.
				as_write(as, "0110 0110 : 0110 1000 : %16d", src.imm);
				break;
			case 4:
				as_write(as, "0110 1000 : %32d", src.imm);
				break;
			default:
				fprintf(stderr, "as_push(): Immediate can only be 1, 2 or 4 bytes, sorry.\n");
				abort();
		}
		return as_slot_for_last_instr(as, src.bytes, ASM_ARG_IMM);
	} else if ( src.bytes == 8 && as_write_modrm(as, WMRM_FIXED_OP_SIZE, "1111 1111", as_op_code(0b110), src, NULL, NULL) ) {
		// wordregister  0101 0101 : 0100 000B : 1111 1111 : 11  110 reg16
		// qwordregister             0100 W00B : 1111 1111 : 11  110 reg64
		// memory16      0101 0101 : 0100 000B : 1111 1111 : mod 110 r/m
		// memory64                  0100 W00B : 1111 1111 : mod 110 r/m
		return as_invalid_slot();
	}
	
	fprintf(stderr, "as_push(): unsupported arg type!\n");
	abort();
	return as_invalid_slot();
}

void as_pop(asm_p as, asm_arg_t dest) {
	// Volume 2C - Instruction Set Reference, p100
	// POP only supports 2 and 8 byte arguments (just like PUSH without immediates)
	if ( dest.bytes == 8 && as_write_modrm(as, WMRM_FIXED_OP_SIZE, "1000 1111", as_op_code(0b000), dest, NULL, NULL) )
		return;
	
	fprintf(stderr, "as_pop(): unsupported arg type!\n");
	abort();
}



//
// Binary Arithmetic Instructions
//

asm_slot_t as_add(asm_p as, asm_arg_t dest, asm_arg_t src) {
	// Volume 2C - Instruction Set Reference, p90
	if (src.type == ASM_ARG_IMM) {
		// 1000 00sw : mm 000, but for now we set s = 0 since we only have unsigned regs
		// Presence of REX.W forces sign extention to 64 bits... (Combined Volumes, p1348)
		// Without it it's written into the 32 bit registers which zeros out the upper 32
		// bits of that register. So effectively we can't disable sign-extention in 64 bit
		// mode... :(
		if (src.imm & 0x80000000) {
			fprintf(stderr, "as_add(): can't encode unsigned immediates larget than 31 bits!\n");
			abort();
		}
		if (dest.bytes < 2) {
			fprintf(stderr, "as_add(): can't put immediates into smaller register!\n");
			abort();
		}
		as_write_modrm(as, 0, "1000 000w", as_op_code(0b000), dest, NULL, NULL);
		as_write(as, "%32d", src.imm);
		return as_slot_for_last_instr(as, 4, ASM_ARG_IMM);
	} else if ( as_write_modrm(as, 0, "0000 00dw", dest, src, NULL, NULL) ) {
		return as_invalid_slot();
	}
	
	fprintf(stderr, "as_add(): unsupported arg combination!\n");
	abort();
	return as_invalid_slot();
}

asm_slot_t as_sub(asm_p as, asm_arg_t dest, asm_arg_t src) {
	// Volume 2C - Instruction Set Reference, p106
	if (src.type == ASM_ARG_IMM) {
		// 1000 00sw : mm 101, but for now we set s = 0 since we only have unsigned regs
		// Presence of REX.W forces sign extention to 64 bits... (Combined Volumes, p1348)
		// Without it it's written into the 32 bit registers which zeros out the upper 32
		// bits of that register. So effectively we can't disable sign-extention in 64 bit
		// mode... :(
		if (src.imm & 0x80000000) {
			fprintf(stderr, "as_sub(): can't encode unsigned immediates larget than 31 bits!\n");
			abort();
		}
		if (dest.bytes < 2) {
			fprintf(stderr, "as_sub(): can't put immediates into smaller register!\n");
			abort();
		}
		as_write_modrm(as, 0, "1000 000w", as_op_code(0b101), dest, NULL, NULL);
		as_write(as, "%32d", src.imm);
		return as_slot_for_last_instr(as, 4, ASM_ARG_IMM);
	} else if ( as_write_modrm(as, 0, "0010 10dw", dest, src, NULL, NULL) ) {
		return as_invalid_slot();
	}
	
	fprintf(stderr, "as_sub(): unsupported arg combination!\n");
	abort();
	return as_invalid_slot();
}

void as_mul(asm_p as, asm_arg_t src) {
	// Volume 2C - Instruction Set Reference, p99
	if ( as_write_modrm(as, 0, "1111 011w", as_op_code(0b100), src, NULL, NULL) )
		return;
	
	fprintf(stderr, "as_mul(): unsupported arg combination!\n");
	abort();
}

void as_div(asm_p as, asm_arg_t src) {
	// Volume 2C - Instruction Set Reference, p94
	// Divide RDX:RAX by qwordregister  0100 100B : 1111 0111 : 11  110 qwordreg
	// Divide RDX:RAX by memory64       0100 10XB : 1111 0111 : mod 110 r/m
	if ( as_write_modrm(as, 0, "1111 011w", as_op_code(0b110), src, NULL, NULL) )
		return;
	
	fprintf(stderr, "as_div(): unsupported arg combination!\n");
	abort();
}

asm_slot_t as_cmp(asm_p as, asm_arg_t arg1, asm_arg_t arg2) {
	// Volume 2C - Instruction Set Reference, p93 (B.2.1 General Purpose Instruction Formats and Encodings for 64-Bit Mode)
	if (arg2.type == ASM_ARG_IMM) {
		if (arg1.bytes < 2) {
			fprintf(stderr, "as_cmp(): can't compare immediates with smaller register!\n");
			abort();
		}
		// 0100 00XB 1000 00sw : mod 111 r/m : imm
		as_write_modrm(as, 0, "1000 00sw", as_op_code(0b111), arg1, NULL, (asm_var_t[]){
			{ "s",  0 },  // B.1.4.4 Sign-Extend (s) Bit, Volume 2C p76
			{ NULL, 0 }
		});
		as_write(as, "%32d", arg2.imm);
		return as_slot_for_last_instr(as, 4, ASM_ARG_IMM);
	} else if ( as_write_modrm(as, 0, "0011 10dw", arg1, arg2, NULL, NULL) ) {
		// memory64 with qwordregister  0100 1RXB : 0011 1001  : mod qwordreg r/m
		// qwordregister with memory64  0100 1RXB : 0011 101w1 : mod qwordreg r/m
		//                                                   |-- PROBABLY ERROR IN DOCS
		return as_invalid_slot();
	}
	
	fprintf(stderr, "as_cmp(): unsupported arg combination!\n");
	abort();
	return as_invalid_slot();
}


//
// Bit and Byte Instructions
//

void as_set_cc(asm_p as, asm_cond_t condition_code, asm_arg_t dest) {
	// Volume 2C - Instruction Set Reference, p104
	// 
	// Combined Volumes 1, 2ABC, 3ABC, p1308
	// In IA-64 mode, the operand size is fixed at 8 bits. Use of REX prefix enable uniform addressing to additional byte
	// registers. Otherwise, this instruction’s operation is the same as in legacy mode and compatibility mode.
	// 
	// The byte versions of RSI, RDI, RBP and RSP (SIL, DIL, BPL, SPL) can only
	// be encoded by an empty REX byte. We we omit this we get AH, BH, CH and DH.
	// So for now we force the precense of a REX byte with a flag.
	if ( dest.bytes != 1 ) {
		fprintf(stderr, "as_set_cc(): target has to be an byte (8 bit) r/m!\n");
		abort();
	}
	bool result = as_write_modrm(as, WMRM_FIXED_OP_SIZE, "0000 1111 : 1001 tttt", as_op_code(0b000), dest, NULL, (asm_var_t[]){
		{ "tttt", condition_code },
		{ NULL, 0 }
	});
	if (result)
		return;
	
	fprintf(stderr, "as_set_cc(): unsupported arg combination!\n");
	abort();
}


//
// Control Transfer Instructions
//

asm_slot_t as_jmp(asm_p as, asm_arg_t target) {
	if (target.type == ASM_ARG_DISP) {
		as_write(as, "1110 1001 : %32d", target.disp);
		return as_slot_for_last_instr(as, 4, ASM_ARG_DISP);
	} else if ( target.bytes == 8 && as_write_modrm(as, WMRM_FIXED_OP_SIZE, "1111 1111", as_op_code(0b100), target, NULL, NULL) ) {
		// Combined Volumes 1, 2ABC, 3ABC, p856:
		// In 64-Bit Mode — The instruction’s operation size is fixed at 64 bits. If a selector points to a gate, then RIP equals
		// the 64-bit displacement taken from gate; else RIP equals the zero-extended offset from the far pointer referenced
		// in the instruction.
		// 
		// register indirect  0100 W00B : 1111 1111 : 11  100 reg
		// memory indirect    0100 W0XB : 1111 1111 : mod 100 r/m
		return as_invalid_slot();
	}
	
	fprintf(stderr, "as_jmp(): unsupported arg type!\n");
	abort();
	return as_invalid_slot();
}

asm_slot_t as_jmp_cc(asm_p as, asm_cond_t condition_code, asm_arg_t displacement) {
	if (displacement.type != ASM_ARG_DISP) {
		fprintf(stderr, "as_jmp_cc(): Arg has to be an as_disp()!\n");
		abort();
	}
	
	as_write(as, "0000 1111 : 1000 tttt : %32d", condition_code, displacement.disp);
	return as_slot_for_last_instr(as, 4, ASM_ARG_DISP);
}


//
// Function call instructions
//

asm_slot_t as_call(asm_p as, asm_arg_t target) {
	// Volume 2C - Instruction Set Reference, p93
	if (target.type == ASM_ARG_DISP) {
		as_write(as, "1110 1000 : %32d", target.disp);
		return as_slot_for_last_instr(as, 4, ASM_ARG_DISP);
	} else if ( target.bytes == 8 && as_write_modrm(as, WMRM_FIXED_OP_SIZE, "1111 1111", as_op_code(0b010), target, NULL, NULL) ) {
		return as_invalid_slot();
	}
	
	fprintf(stderr, "as_call(): unsupported arg type!\n");
	abort();
	return as_invalid_slot();
}

asm_slot_t as_ret(asm_p as, int16_t stack_size_to_pop) {
	// Volume 2C - Instruction Set Reference, p102
	if (stack_size_to_pop == 0) {
		as_write(as, "1100 0011");
		return as_invalid_slot();
	}
	
	as_write(as, "1100 0010 : %16d", stack_size_to_pop);
	return as_slot_for_last_instr(as, 2, ASM_ARG_IMM);
}

void as_enter(asm_p as, int16_t stack_size, int8_t level) {
	// Volume 2C - Instruction Set Reference, p94
	as_write(as, "1100 1000 : %16d : %8d", stack_size, level);
}

void as_leave(asm_p as) {
	// Volume 2C - Instruction Set Reference, p96
	as_write(as, "1100 1001");
}


//
// Misc instructions
//

void as_syscall(asm_p as) {
	// Volume 2C - Instruction Set Reference, p107 (B.2.1 General Purpose Instruction Formats and Encodings for 64-Bit Mode)
	as_write(as, "0000 1111 : 0000 0101");
}