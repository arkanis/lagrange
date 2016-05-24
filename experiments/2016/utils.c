#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "utils.h"


//
// Not zero-terminated strings
//

void str_free(str_p str) {
	free(str->ptr);
	str->ptr = NULL;
	str->len = 0;
}

void str_putc(str_p str, char c) {
	str->len++;
	str->ptr = realloc(str->ptr, str->len);
	str->ptr[str->len - 1] = c;
}


//
// File I/O
//

str_t str_fload(const char* filename) {
	str_t str = { 0, NULL };
	long filesize = 0;
	char* data = NULL;
	int error = -1;
	
	FILE* f = fopen(filename, "rb");
	if (f == NULL)
		return str;
	
	if ( fseek(f, 0, SEEK_END)              == -1       ) goto fail;
	if ( (filesize = ftell(f))              == -1       ) goto fail;
	if ( fseek(f, 0, SEEK_SET)              == -1       ) goto fail;
	if ( (data = malloc(filesize))          == NULL     ) goto fail;
	// TODO: proper error detection for fread and get proper error code with ferror
	if ( (long)fread(data, 1, filesize, f)  != filesize ) goto free_and_fail;
	fclose(f);
	
	str.len = filesize;
	str.ptr = data;
	return str;
	
	free_and_fail:
		error = errno;
		free(data);
	
	fail:
		if (error == -1)
			error = errno;
		fclose(f);
	
	errno = error;
	return str;
}