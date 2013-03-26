#include <stdio.h>
#include "scanner.h"

int main(){
	scanner_t scan = scanner_new("test.m");
	
	token_t tok;
	do {
		tok = scanner_read(&scan);
		printf("%s: %.*s\n", token_names[tok.type], (int)tok.length, tok.text);
	} while(tok.type != T_EOF);
	
	scanner_destroy(&scan);
}