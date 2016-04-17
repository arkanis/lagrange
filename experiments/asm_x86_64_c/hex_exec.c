/**
 * This experiment executes raw data as instructions.
 * 
 * It allocates an executable page, fills it with instructions of a function
 * (hex_exec_sample_func.c) and executes the data via a function pointer.
 */

#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int decimal_char_to_value(char c){
	if (c >= 48 && c <= 57)
		return c - 48;
	else if (c >= 65 && c <= 70)
		return c - 65 + 10;
	else if (c >= 97 && c <= 102)
		return c - 97 + 10;
	else
		return -1;
}

void init_mem_from_hex(uint8_t* memory, const char* hex_code){
	bool got_first_char = false;
	unsigned char byte;
	size_t mi = 0;
	
	for(size_t i = 0; i < strlen(hex_code); i++){
		int c = hex_code[i];
		
		if ( isspace(c) )
			continue;
		
		if (!got_first_char) {
			byte = decimal_char_to_value(c) << 4;
			got_first_char = true;
		} else {
			byte += decimal_char_to_value(c);
			got_first_char = false;
			
			memory[mi] = byte;
			mi++;
		}
	}
}

typedef int (*call_t)();

int main(){
	size_t page_size = 4096;
	uint8_t* page = mmap(NULL, page_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	assert(page != NULL);
	
	// Hex code for instructions taken from
	// hex_exec_sample_func.c disassembly
	init_mem_from_hex(page, "\
		55 \
		48 89 e5 \
		b8 05 00 00 00 \
		5d \
		c3 \
	");
	
	call_t func = (call_t) page;
	int x = func();
	printf("x: %d\n", x);
	
	munmap(page, page_size);
	return 0;
}