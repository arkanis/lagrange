#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <glob.h>
#include "../asm.h"

#define SLIM_TEST_IMPLEMENTATION
#include "slim_test.h"
#include "utils.c"


void test_samples() {
	glob_t samples;
	int error = glob("tests/samples/*.lg", GLOB_ERR, NULL, &samples);
	st_check_int(error, 0);
	
	for(size_t i = 0; i < samples.gl_pathc; i++) {
		char* filename = samples.gl_pathv[i];
		//printf("file: %s\n", filename);
		
		int expected_status = -1;
		char* expected_output = NULL;
		
		FILE* f = fopen(filename, "r");
		char buffer[512];
		char* line = NULL;
		while ( (line = fgets(buffer, sizeof(buffer), f)) != NULL ) {
			if ( strncmp(line, "// status: ", 11) == 0 ) {
				sscanf(line, "// status: %d", &expected_status);
			} else if ( strncmp(line, "// output: ", 11) == 0 ) {
				sscanf(line, "// output: %m[^\n]", &expected_output);
			}
		}
		fclose(f);
		//printf("expected status: %d, output: %s\n", expected_status, expected_output);
		
		char command[512];
		snprintf(command, sizeof(command), "./main %s", filename);
		//printf("compile command: %s\n", command);
		system(command);
		
		char* output = NULL;
		int status_code = run_and_delete("main.elf", "./main.elf", &output);
		if (expected_status != -1) {
			st_check_int(status_code, expected_status);
		}
		if (expected_output != NULL) {
			st_check_str(output, expected_output);
		}
		
		free(output);
		free(expected_output);
	}
	
	globfree(&samples);
}


int main() {
	st_run(test_samples);
	return st_show_report();
}