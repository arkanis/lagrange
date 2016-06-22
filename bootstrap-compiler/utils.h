#pragma once

#include <stdbool.h>
#include <stdlib.h>


//
// Simple generic variable length list
//

#define list_t(content_type_t)   struct { size_t len; content_type_t* ptr; }

#define list_new(list_ptr)  do {  \
    (list_ptr)->ptr = NULL;       \
    (list_ptr)->len = 0;          \
} while(0)

#define list_destroy(list_ptr)  do {  \
    free( (list_ptr)->ptr );          \
    (list_ptr)->ptr = NULL;           \
    (list_ptr)->len = 0;              \
} while(0)

#define list_resize(list_ptr, new_len)  do {                                                   \
    (list_ptr)->len = (new_len);                                                               \
    (list_ptr)->ptr = realloc((list_ptr)->ptr, (list_ptr)->len * sizeof((list_ptr)->ptr[0]));  \
} while(0)

#define list_append(list_ptr, value)  do {           \
    list_resize((list_ptr), (list_ptr)->len + 1);    \
    (list_ptr)->ptr[(list_ptr)->len - 1] = (value);  \
} while(0)

#define list_shift(list_ptr, n)  if ( (list_ptr)->len >= n ) {  \
    for(size_t i = 0; i < (list_ptr)->len - (n); i++)           \
        (list_ptr)->ptr[i] = (list_ptr)->ptr[i + (n)];          \
    list_resize((list_ptr), (list_ptr)->len - (n));             \
}



//
// Not zero-terminated strings
// 
// The length is an int so we can just dump it into printf as string field
// precision without casting.
//

typedef struct {
	int   len;
	char* ptr;
} str_t, *str_p;

static inline str_t str_new(char* ptr, int len) { return (str_t){ len, ptr }; }
static inline str_t str_empty() { return (str_t){ 0, NULL }; }
str_t               str_from_c(char* zero_terminated_string);
void                str_free(str_p str);

void str_putc(str_p str, char c);
bool str_eq(const str_p a, const str_p b);
bool str_eqc(const str_p a, const char* b);



//
// File I/O
//

str_t str_fload(const char* filename);