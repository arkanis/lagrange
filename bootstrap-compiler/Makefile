CFLAGS = -std=c99 -g -Werror -Wall -Wextra -Wno-unused-parameter -Wno-unused-function

tests/utils_test: tests/utils_test.c utils.o
tests/asm_test: tests/asm_test.c asm.o

# clean target for all directories, ensures that the ignore files are properly maintained.
clean:
	rm -fr `tr '\n' ' ' < .gitignore`