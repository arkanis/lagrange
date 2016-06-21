GCC_OPTS = -std=gnu99 -g -Wall

shift_reduce_parser: shift_reduce_parser.h shift_reduce_parser.c
	gcc $(GCC_OPTS) shift_reduce_parser.c -o shift_reduce_parser

grammar: grammar.h grammar.c
	gcc $(GCC_OPTS) grammar.c pbl/libpbl.a -o grammar

clean:
	rm -rf grammar shift_reduce_parser

# pbl
# cd pbl
# make libpbl.a