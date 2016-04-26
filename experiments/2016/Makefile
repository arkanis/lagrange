CFLAGS = -std=gnu99 -Werror -Wall -Wextra -Wno-unused-parameter -g

main: lexer.o parser.o ast.o
tests/lexer_test: tests/lexer_test.c lexer.o

# clean target for all directories, ensures that the ignore files are properly maintained.
clean:
	rm -fr `tr '\n' ' ' < .gitignore`