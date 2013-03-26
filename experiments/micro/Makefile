GCC_ARGS = -Wall -std=gnu99 -g

scanner_test: scanner_test.c scanner.o
	gcc $(GCC_ARGS) scanner_test.c scanner.o -o scanner_test

scanner.o: scanner.h scanner.c
	gcc $(GCC_ARGS) -c scanner.c


clean:
	rm -f *.o *_test