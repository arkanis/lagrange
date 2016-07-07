CFLAGS = -std=c99 -g -Werror -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
TESTS  = $(patsubst %.c,%,$(wildcard tests/*_test.c))

all: tests lgc

# Lagrange compiler binary
lgc: utils.o tokenizer.o parser.o

# Tests
tests: $(TESTS)
	$(foreach test,$(TESTS),$(shell $(test)))

tests/utils_test: tests/utils_test.c utils.o
tests/asm_test: tests/asm_test.c asm.o tests/disassembler_utils.o
tests/tokenizer_test: tests/tokenizer_test.c tokenizer.o utils.o


# clean target for all directories, ensures that the ignore files are properly maintained.
clean:
	rm -fr `tr '\n' ' ' < .gitignore`