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
	
	size_t source_size = 0;
	char* source = sgl_fload(argv[1], &source_size);
	tokenized_file_p file = tokenize_str(source, source_size, argv[1], stderr);
	
	for(size_t i = 0; i < file->token_count; i++) {
		token_p t = &file->tokens[i];
		if (t->type == T_COMMENT || t->type == T_WS) {
			printf("%.*s", t->size, t->source);
		} else {
			char buffer[128];
			token_dump(t, buffer, sizeof(buffer));
			printf("%s ", buffer);
		}
	}
	printf("\n");
	
	parse_module(file);
	
	tokenized_file_free(file);
	free(source);
	
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
