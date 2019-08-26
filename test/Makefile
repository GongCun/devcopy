ROOT = ..
include $(ROOT)/Make.defines
EXTRALIBS = -lgdbm_compat -lgdbm -lz

CPPFLAGS = -I$(LDDIR) -I$(ROOT)

PROGS = error-test gdbm-test ktree-test

all: $(PROGS)

$(PROGS): %: %.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)
