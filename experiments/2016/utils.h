#pragma once

#include <stddef.h>


//
// Simple variable length buffer
//

#define list_t(content_type_t)   struct { size_t len; content_type_t* ptr; }
#define list_append(list, value)  do {                                         \
	(list)->len++;                                                             \
	(list)->ptr = realloc((list)->ptr, (list)->len * sizeof((list)->ptr[0]));  \
	(list)->ptr[(list)->len - 1] = (value);                                    \
} while(0)


//
// Not zero-terminated strings
//

typedef struct {
	int   len;
	char* ptr;
} str_t, *str_p;

void str_free(str_p str);

void str_putc(str_p str, char c);


//
// File I/O
//

str_t str_fload(const char* filename);