#include <string.h>
#include "../ast.h"

#define SLIM_TEST_IMPLEMENTATION
#include "slim_test.h"


void test_iterator() {
	node_p f1 = node_alloc(NT_FUNC);
		node_p i11 = node_alloc_append(NT_ARG, f1, &f1->func.in);
		node_p i12 = node_alloc_append(NT_ARG, f1, &f1->func.in);
		
		node_p o11 = node_alloc_append(NT_ARG, f1, &f1->func.out);
		node_p o12 = node_alloc_append(NT_ARG, f1, &f1->func.out);
		node_p o13 = node_alloc_append(NT_ARG, f1, &f1->func.out);
		
		node_p b11 = node_alloc_append(NT_VAR, f1, &f1->func.body);
		node_p b12 = node_alloc_append(NT_VAR, f1, &f1->func.body);
			node_p id131  = node_alloc_set(NT_ID, b12, &b12->var.type);
				id131->id.name = str_from_c("foo");
			node_p int132 = node_alloc_set(NT_INTL, b12, &b12->var.value);
				int132->intl.value = 17;
		node_p b13 = node_alloc_append(NT_IF, f1, &f1->func.body);
	
	
	ast_it_t it;
	it = ast_start(f1);
		st_check(it.node == i11);
	it = ast_next(f1, it);
		st_check(it.node == i12);
	
	it = ast_next(f1, it);
		st_check(it.node == o11);
	it = ast_next(f1, it);
		st_check(it.node == o12);
	it = ast_next(f1, it);
		st_check(it.node == o13);
	
	it = ast_next(f1, it);
		st_check(it.node == b11);
	it = ast_next(f1, it);
		st_check(it.node == b12);
	it = ast_next(f1, it);
		st_check(it.node == b13);
	
	it = ast_next(f1, it);
		st_check_null(it.node);
	it = ast_next(f1, it);
		st_check_null(it.node);
	it = ast_next(f1, it);
		st_check_null(it.node);
}


int main() {
	st_run(test_iterator);
	return st_show_report();
}