#include "../utils.h"

#define SLIM_TEST_IMPLEMENTATION
#include "slim_test.h"


//
// Generic list
//

void test_list_new_and_destroy() {
	list_t(int) list;
	
	list_new(&list);
	st_check_int(list.len, 0);
	st_check_null(list.ptr);
	
	list_destroy(&list);
}

void test_list_resize() {
	list_t(int) list;
	list_new(&list);
	
	list_resize(&list, 100);
	st_check_int(list.len, 100);
	st_check_not_null(list.ptr);
	
	for(size_t i = 0; i < list.len; i++)
		list.ptr[i] = i;
	for(size_t i = 0; i < list.len; i++)
		st_check_int(list.ptr[i], (int)i);
	
	list_resize(&list, 5);
	st_check_int(list.len, 5);
	st_check_not_null(list.ptr);
	
	for(size_t i = 0; i < list.len; i++)
		list.ptr[i] = i;
	for(size_t i = 0; i < list.len; i++)
		st_check_int(list.ptr[i], (int)i);
	
	list_destroy(&list);
	st_check_int(list.len, 0);
	st_check_null(list.ptr);
}

void test_list_append() {
	list_t(int) list;
	list_new(&list);
	
	list_append(&list, 0);
	list_append(&list, 1);
	list_append(&list, 2);
	
	st_check_int(list.len, 3);
	st_check_not_null(list.ptr);
	
	st_check_int(list.ptr[0], 0);
	st_check_int(list.ptr[1], 1);
	st_check_int(list.ptr[2], 2);
	
	list_destroy(&list);
}

void test_list_shift() {
	list_t(int) list;
	list_new(&list);
	
	list_append(&list, 0);
	list_append(&list, 1);
	list_append(&list, 2);
	list_append(&list, 3);
	st_check_int(list.len, 4);
	
	list_shift(&list, 1);
	st_check_int(list.len, 3);
	st_check_int(list.ptr[0], 1);
	st_check_int(list.ptr[1], 2);
	st_check_int(list.ptr[2], 3);
	
	list_shift(&list, 2);
	st_check_int(list.len, 1);
	st_check_int(list.ptr[0], 3);
	
	list_shift(&list, 1);
	st_check_int(list.len, 0);
	
	list_destroy(&list);
}


//
// Not zero-terminated strings
//

void test_str_from_mem_and_free() {
	char* s = "xyz";
	
	str_t str = str_from_mem(s, 3);
	st_check_int(str.len, 3);
	st_check(str.ptr == s);
	
	str = str_from_c(s);
	st_check_int(str.len, 3);
	st_check(str.ptr == s);
	
	str = str_empty();
	st_check_int(str.len, 0);
	st_check_null(str.ptr);
	
	s = malloc(2);
	s[0] = 'a';
	s[1] = 'b';
	
	str = str_from_mem(s, 2);
	st_check_int(str.len, 2);
	st_check(str.ptr == s);
	
	str_free(&str);
	st_check_int(str.len, 0);
	st_check_null(str.ptr);	
}

void test_str_putc() {
	str_t str = str_empty();
	
	str_putc(&str, 'u');
	st_check_int(str.len, 1);
	st_check_not_null(str.ptr);
	st_check_int(str.ptr[0], 'u');
	
	str_putc(&str, 'v');
	st_check_int(str.len, 2);
	st_check_not_null(str.ptr);
	st_check_int(str.ptr[0], 'u');
	st_check_int(str.ptr[1], 'v');
	
	str_putc(&str, 'w');
	st_check_int(str.len, 3);
	st_check_not_null(str.ptr);
	st_check_int(str.ptr[0], 'u');
	st_check_int(str.ptr[1], 'v');
	st_check_int(str.ptr[2], 'w');
	
	str_free(&str);
}

void test_str_eq_and_eqc() {
	str_t a = str_from_c("foo");
	str_t b = str_from_c("bar");
	str_t c = str_empty();
	bool result;
	
	str_putc(&c, 'f');
	str_putc(&c, 'o');
	str_putc(&c, 'o');
	
	result = str_eq(&a, &c);
	st_check_int(result, true);
	
	result = str_eq(&a, &b);
	st_check_int(result, false);
	
	result = str_eqc(&a, "foo");
	st_check_int(result, true);
	
	result = str_eqc(&b, "foo");
	st_check_int(result, false);
	
	str_free(&c);
}


//
// File I/O
//

void test_str_fload() {
	str_t content = str_fload(__FILE__);
	st_check(content.len > 0);
	st_check_not_null(content.ptr);
	str_free(&content);
	
	content = str_fload("not_existing_file.foobar");
	st_check_int(content.len, 0);
	st_check_null(content.ptr);
}


int main() {
	st_run(test_list_new_and_destroy);
	st_run(test_list_resize);
	st_run(test_list_append);
	st_run(test_list_shift);
	st_run(test_str_from_mem_and_free);
	st_run(test_str_putc);
	st_run(test_str_eq_and_eqc);
	st_run(test_str_fload);
	return st_show_report();
}