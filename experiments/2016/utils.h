#pragma once

#include <stddef.h>
#include <stdbool.h>


//
// Simple variable length buffer
//

#define list_t(content_type_t)   struct { size_t len; content_type_t* ptr; }

#define list_new(list_ptr)  do {  \
	(list_ptr)->ptr = NULL;       \
	(list_ptr)->len = 0;          \
} while(0)

#define list_free(list_ptr)  do {  \
	free( (list_ptr)->ptr );       \
	(list_ptr)->ptr = NULL;        \
	(list_ptr)->len = 0;           \
} while(0)

#define list_resize(list_ptr, new_len)  do {                                                   \
	(list_ptr)->len = new_len;                                                                 \
	(list_ptr)->ptr = realloc((list_ptr)->ptr, (list_ptr)->len * sizeof((list_ptr)->ptr[0]));  \
} while(0)

#define list_append(list_ptr, value)  do {           \
    list_resize((list_ptr), (list_ptr)->len + 1);    \
	(list_ptr)->ptr[(list_ptr)->len - 1] = (value);  \
} while(0)

#define list_shift(list_ptr, n)  do {                  \
	for(ssize_t i = (list_ptr)->len - 1; i >= n; i--)  \
		(list_ptr)->ptr[i] = (list_ptr)->ptr[i-n];     \
	list_resize((list_ptr), (list_ptr)->len - n);      \
} while(0)


//
// Not zero-terminated strings
//

typedef struct {
	int   len;
	char* ptr;
} str_t, *str_p;

str_t str_from_c(char* zero_terminated_string);
void  str_free(str_p str);

void str_putc(str_p str, char c);
bool str_eq(str_p a, str_p b);
bool str_eqc(str_p a, const char* b);


//
// File I/O
//

str_t str_fload(const char* filename);