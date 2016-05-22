CFLAGS = -std=gnu99 -Werror -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -g

main: lexer.o parser.o ast.o reg_alloc.o asm.o
tests/lexer_test: tests/lexer_test.c lexer.o
tests/asm_test: tests/asm_test.c asm.o

# clean target for all directories, ensures that the ignore files are properly maintained.
clean:
	rm -fr `tr '\n' ' ' < .gitignore`