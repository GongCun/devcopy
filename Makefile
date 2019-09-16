ROOT = .
OS = $(shell uname -s | tr 'A-Z' 'a-z')
include $(ROOT)/Make.defines.$(OS)
EXTRALIBS = -lgdbm_compat -lgdbm

CPPFLAGS = -I$(LDDIR) -I$(ROOT)

OBJS = devcopy-function.o
PROGS = devcopy devcopy-hash devcopy-delta

all:	$(PROGS) devcopy-version

$(PROGS): %: %.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

devcopy-version: devcopy-version.o $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(TEMPFILES) $(PROGS)
