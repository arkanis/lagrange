#pragma once
#include "../asm.h"

int run_and_delete(const char* elf_filename, const char* command, char** output);
char* disassemble(asm_p as);