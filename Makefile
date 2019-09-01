ROOT = .
OS = $(shell uname -s | tr 'A-Z' 'a-z')
include $(ROOT)/Make.defines.$(OS)
EXTRALIBS = -lgdbm_compat -lgdbm -lz

CPPFLAGS = -I$(LDDIR) -I$(ROOT)

PROGS = devcopy devcopy-hash devcopy-delta

all:	$(PROGS)

$(PROGS): %: %.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)
