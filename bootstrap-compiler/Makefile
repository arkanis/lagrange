CFLAGS = -std=c99 -g -Werror -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
TESTS  = $(patsubst %.c,%,$(wildcard tests/*_test.c))

all: tests lgc

# Lagrange compiler binary
lgc: utils.o tokenizer.o parser.o ast.o operators.o namespaces.o

# Tests
tests: $(TESTS)
	$(foreach test,$(TESTS),$(shell $(test)))

tests/utils_test: utils.o
tests/asm_test: asm.o tests/disassembler_utils.o tokenizer.o utils.o
tests/tokenizer_test: tokenizer.o utils.o ast.o namespaces.o
tests/ast_test: ast.o utils.o namespaces.o tokenizer.o
tests/parser_test: tokenizer.o parser.o ast.o utils.o namespaces.o
tests/resolve_uops_test: tokenizer.o parser.o ast.o utils.o operators.o namespaces.o


# clean target for all directories, ensures that the ignore files are properly maintained.
clean:
	rm -fr `tr '\n' ' ' < .gitignore`