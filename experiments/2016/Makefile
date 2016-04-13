CFLAGS = -std=c99 -Werror -Wall -Wextra -g

main: lexer.o parser.o

# clean target for all directories, ensures that the ignore files are properly maintained.
clean:
	rm -fr `tr '\n' ' ' < .gitignore`