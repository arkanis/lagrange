#include "../common.h"

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
			node_p id131  = node_alloc_set(NT_ID, b12, &b12->var.type_expr);
				id131->id.name = str_from_c("int");
			node_p bdg132 = node_alloc_append(NT_BINDING, b12, &b12->var.bindings);
				bdg132->binding.name = str_from_c("x");
				node_p int132 = node_alloc_set(NT_INTL, bdg132, &bdg132->binding.value);
					int132->intl.value = 17;
		node_p b13 = node_alloc_append(NT_IF_STMT, f1, &f1->func.body);
	
	
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

void test_ast_replace_node() {
	node_p f1 = node_alloc(NT_FUNC);
		node_p i11 = node_alloc_append(NT_ARG, f1, &f1->func.in);
		node_p i12 = node_alloc_append(NT_ARG, f1, &f1->func.in);
	
	node_p r1 = node_alloc(NT_ARG);
	node_p r2 = node_alloc(NT_ARG);
	
	ast_it_t it;
	it = ast_start(f1);
		st_check(it.node == i11);
		ast_replace_node(f1, it, r1);
		st_check(it.node == i11);  // don't change the iterator
		st_check(f1->func.in.ptr[0] == r1);
		st_check(f1->func.in.ptr[1] == i12);
	it = ast_next(f1, it);
		st_check(it.node == i12);
		ast_replace_node(f1, it, r2);
		st_check(it.node == i12);
		st_check(f1->func.in.ptr[0] == r1);
		st_check(f1->func.in.ptr[1] == r2);
}


int main() {
	st_run(test_iterator);
	st_run(test_ast_replace_node);
	return st_show_report();
}