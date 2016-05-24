#pragma once

#include <stddef.h>


//
// Simple variable length buffer
//

#define buf_t(content_type_t) struct { size_t len; content_type_t* ptr; }
#define buf_append(buf, value) do {                                    \
	(buf)->len++;                                                       \
	(buf)->ptr = realloc((buf)->ptr, (buf)->len * sizeof((buf)->ptr[0]));  \
	(buf)->ptr[(buf)->len - 1] = (value);                                \
} while(0)


//
// Not zero-terminated strings
//

typedef struct {
	size_t len;
	char*  ptr;
} str_t, *str_p;

void str_free(str_p str);

void str_putc(str_p str, char c);


//
// File I/O
//

str_t str_fload(const char* filename);