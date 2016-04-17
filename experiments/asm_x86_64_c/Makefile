# This file contains scripts that are used during development. Executables and
# object files are build by implicit rules.
CFLAGS = -g -std=gnu99


asm_run: asm
	./asm
	chmod u+x asm_binary.elf
	./asm_binary.elf

asm_dump: asm
	./asm
	objdump -b binary -m i386:x86-64 -M intel-mnemonic -D -w asm_instructions.raw


hex_exec_sample: hex_exec_sample_func.o
	objdump -M intel-mnemonic -d -w hex_exec_sample_func.o
	rm hex_exec_sample_func.o


# Delete everything listed in the .gitignore file, ensures that it's properly maintained.
clean:
	rm -fr `tr '\n' ' ' < .gitignore`