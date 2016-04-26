#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "lexer.h"
#include "parser.h"

void* sgl_fload(const char* filename, size_t* size);

int main(int argc, char** argv) {
	if (argc < 2) {
		fprintf(stderr, "usage: %s source-file\n", argv[0]);
		return 1;
	}
	
	size_t src_len = 0;
	char* src_str = sgl_fload(argv[1], &src_len);
	token_list_p list = lex_str(src_str, src_len, argv[1], stderr);
	
	if (list->error_count > 0) {
		// Just output errors and exit
		for(size_t i = 0; i < list->tokens_len; i++) {
			token_p t = &list->tokens_ptr[i];
			if (t->type == T_ERROR) {
				fprintf(stderr, "%s:%d:%d: %s\n", argv[1], token_line(t), token_col(t), t->str_val);
				token_print_line(stderr, t, 0);
			}
		}
		
		lex_free(list);
		free(src_str);
		return 1;
	}
	
	for(size_t i = 0; i < list->tokens_len; i++) {
		token_p t = &list->tokens_ptr[i];
		//token_print(stdout, t, TP_INLINE_DUMP);
		//printf("\n");
		if (t->type == T_COMMENT || t->type == T_WS) {
			token_print(stdout, t, TP_SOURCE);
		} else if (t->type == T_WS_EOS || t->type == T_EOF) {
			printf(" ");
			token_print(stdout, t, TP_DUMP);
			printf(" ");
		} else {
			token_print(stdout, t, TP_DUMP);
		}
	}
	printf("\n");
	
	
	node_p tree = parse(list, parse_expr);
	printf("\n");
	node_print(tree, stdout, 0);
	//parse_module(list);
	
	lex_free(list);
	free(src_str);
	
	return 0;
}

void* sgl_fload(const char* filename, size_t* size) {
	long filesize = 0;
	char* data = NULL;
	int error = -1;
	
	FILE* f = fopen(filename, "rb");
	if (f == NULL)
		return NULL;
	
	if ( fseek(f, 0, SEEK_END)              == -1       ) goto fail;
	if ( (filesize = ftell(f))              == -1       ) goto fail;
	if ( fseek(f, 0, SEEK_SET)              == -1       ) goto fail;
	if ( (data = malloc(filesize + 1))      == NULL     ) goto fail;
	// TODO: proper error detection for fread and get proper error code with ferror
	if ( (long)fread(data, 1, filesize, f)  != filesize ) goto free_and_fail;
	fclose(f);
	
	data[filesize] = '\0';
	if (size)
		*size = filesize;
	return (void*)data;
	
	free_and_fail:
		error = errno;
		free(data);
	
	fail:
		if (error == -1)
			error = errno;
		fclose(f);
	
	errno = error;
	return NULL;
}
