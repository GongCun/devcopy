CC = xlc
CFLAGS = -O2 -q64
LIB = -lz

PROGS = devcopy devcopy-hash devcopy-delta

all:	$(PROGS)

devcopy: devcopy.c asprintf.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

devcopy-hash: devcopy-hash.c asprintf.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)

devcopy-delta: devcopy-delta.c
	$(CC) $(CFLAGS) -o $@ $^
