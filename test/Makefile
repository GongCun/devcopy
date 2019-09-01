ROOT = ..
OS = $(shell uname -s | tr 'A-Z' 'a-z')
include $(ROOT)/Make.defines.$(OS)
EXTRALIBS = -lgdbm_compat -lgdbm -lz

CPPFLAGS = -I$(LDDIR) -I$(ROOT)

PROGS = error-test gdbm-test ktree-test gdbm-ktree-test

all: $(PROGS)

$(PROGS): %: %.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)
