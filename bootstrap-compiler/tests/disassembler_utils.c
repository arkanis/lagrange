#define _GNU_SOURCE    // for popen
#include <stdio.h>     // for popen
#include <sys/stat.h>  // for chmod
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "disassembler_utils.h"


int run_and_delete(const char* elf_filename, const char* command, char** output) {
	if (elf_filename)
		chmod(elf_filename, S_IRUSR | S_IWUSR | S_IXUSR);
	
	char* output_ptr = output ? *output : NULL;
	size_t output_len = 0;
	
	FILE* child = popen(command, "r");
	char line[512];
	while(!feof(child)) {
		char* result = fgets(line, sizeof(line), child);
		if (result == NULL)
			break;
		size_t line_len = strlen(line);
		
		output_len += line_len;
		// The +1 at the end makes sure we have the space for the zero terminator
		// and also copy it.
		output_ptr = realloc(output_ptr, output_len + 1);
		memcpy(output_ptr + output_len - line_len, line, line_len + 1);
	}
	int exit_info =  pclose(child);
	
	if (elf_filename)
		unlink(elf_filename);
	
	if (output)
		*output = output_ptr;
	return WEXITSTATUS(exit_info);
}

// Useful commands:
// objdump -m i386:x86-64 -M intel-mnemonic -d main.elf
// objdump --wide -m i386:x86-64 -M intel-mnemonic -d if.elf | tail -n +8 | cut -f 3
// objdump --wide --architecture=i386:x86-64 --disassembler-options=intel,intel-mnemonic --disassemble
char* disassemble(asm_p as) {
	as_save_elf(as, 4*1024*1024, 512*1024*1024, "dissassemble.elf");
	char* output = NULL;
	run_and_delete("dissassemble.elf",
		"objdump --architecture=i386:x86-64 --disassembler-options=intel,intel-mnemonic "
			"--wide --disassemble dissassemble.elf | "
		"tail -n +8 | cut -f 3",
		&output
	);
	return output;
}