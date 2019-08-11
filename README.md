devcopy
==========


Compile on AIX:

	# GCC
	gcc -Wall -maix64 -g -pg -o ./devcopy ./devcopy.c ./asprintf.c -lz
	gcc -Wall -maix64 -g -pg -o ./devcopy-hash ./devcopy-hash.c ./asprintf.c -lz
	gcc -Wall -maix64 -g -pg -o ./devcopy-delta ./devcopy-delta.c
    
    # XLC
    xlc -O2 -q64 -o ./devcopy ./devcopy.c ./asprintf.c -lz
    xlc -O2 -q64 -o ./devcopy-hash ./devcopy-hash.c ./asprintf.c -lz
    xlc -O2 -q64 -o ./devcopy-delta ./devcopy-delta.c
