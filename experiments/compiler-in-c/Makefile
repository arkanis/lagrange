CFLAGS = -std=gnu99 -Werror -Wall -Wextra -Wno-unused-parameter -Wno-unused-function -g

main: lexer.o parser.o ast.o reg_alloc.o asm.o utils.o
tests/lexer_test: tests/lexer_test.c lexer.o
tests/asm_test: tests/asm_test.c asm.o
tests/samples_test: tests/samples_test.c asm.o
tests/ast_test: tests/ast_test.c ast.o asm.o utils.o

# clean target for all directories, ensures that the ignore files are properly maintained.
clean:
	rm -fr `tr '\n' ' ' < .gitignore`