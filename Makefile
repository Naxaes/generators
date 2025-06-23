all: coprimes fib lexer primes square

coprimes: build examples/coprimes.c
	cc -std=c99 -O2 -arch x86_64 -Wall -Wextra -pedantic -g -ggdb -I. examples/coprimes.c -o build/coprimes

fib: build examples/fib.c
	cc -std=c99 -O2 -arch x86_64 -Wall -Wextra -pedantic -g -ggdb -I. examples/fib.c -o build/fib

lexer: build examples/lexer.c
	cc -std=c99 -O2 -arch x86_64 -Wall -Wextra -pedantic -g -ggdb -I. examples/lexer.c -o build/lexer

primes: build examples/primes.c
	cc -std=c99 -O2 -arch x86_64 -Wall -Wextra -pedantic -g -ggdb -I. examples/primes.c -o build/primes

square: build examples/square.c
	cc -std=c99 -O2 -arch x86_64 -Wall -Wextra -pedantic -g -ggdb -I. examples/square.c -o build/square

build:
	mkdir -p build