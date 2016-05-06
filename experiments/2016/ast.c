#include <stdlib.h>
#include <stdio.h>
#include "ast.h"

void node_print(node_p node, FILE* output, int level) {
	fprintf(output, "%s: ", node->spec->name);
	if (node->spec->print)
		node->spec->print(node, output);
	else
		fprintf(output, "\n");
	
	for(size_t i = 0; i < node->spec->members.len; i++) {
		member_spec_p member = &node->spec->members.ptr[i];
		switch(member->type) {
			case MT_NODE: {
				// Here we calculate the position of a node_p (position of the member),
				// so in effect we got a node_p*. When we dereference it we got the
				// actual value of the property (the pointer to the child node).
				node_p member_node = *(node_p*)( ((uint8_t*)node) + member->offset );
				fprintf(output, "%*s%s: ", (level+1)*2, "", member->name);
				node_print(member_node, output, level+1);
				} break;
			case MT_LIST: {
				buf_t(node_p)* member_list = (void*)( ((uint8_t*)node) + member->offset );
				for(size_t j = 0; j < member_list->len; j++) {
					fprintf(output, "%*s%s[%zu]: ", (level+1)*2, "", member->name, j);
					node_print(member_list->ptr[j], output, level+1);
				}
				} break;
		}
	}
}

node_p node_iterate(node_p node, uint32_t level, node_it_func_t func) {
	if (node->spec->members.len == 0) {
		// Node has no children
		return func(node, level, NI_LEAF);
	}
	
	// Node has children
	node = func(node, level, NI_PRE);
	
	for(size_t i = 0; i < node->spec->members.len; i++) {
		member_spec_p member = &node->spec->members.ptr[i];
		switch(member->type) {
			case MT_NODE: {
				// Here we calculate the position of a node_p (position of the member),
				// so in effect we got a node_p*. When we dereference it we got the
				// actual value of the property (the pointer to the child node).
				node_p* member_node = (node_p*)( (void*)node + member->offset );
				//node_p member_node = *(node_p*)( ((uint8_t*)node) + member->offset );
				*member_node = node_iterate(*member_node, level + 1, func);
				} break;
			case MT_LIST: {
				buf_t(node_p)* member_list = (void*)( ((uint8_t*)node) + member->offset );
				for(size_t j = 0; j < member_list->len; j++)
					member_list->ptr[j] = node_iterate(member_list->ptr[j], level + 1, func);
				} break;
		}
	}
	
	return func(node, level, NI_POST);
}

/*
int main() {
	uops_p test = node_alloc(uops, NULL);
		str_p s1 = node_alloc(str, test); s1->value = "foo";
		buf_append(test->list, (node_p)s1);
		str_p s2 = node_alloc(str, test); s2->value = "bar";
		buf_append(test->list, (node_p)s2);
		uops_p u1 = node_alloc(uops, test);
		buf_append(test->list, (node_p)u1);
			str_p us1 = node_alloc(str, u1); us1->value = "u foo";
			buf_append(u1->list, (node_p)us1);
			str_p us2 = node_alloc(str, u1); us2->value = "u bar";
			buf_append(u1->list, (node_p)us2);
	
	node_print(&test->node, stdout, 0);
	
	return 0;
}
*/