ROOT = ..
include $(ROOT)/Make.defines
EXTRALIBS = -lgdbm -lz

CPPFLAGS = -I$(LDDIR) -I$(ROOT)

PROGS = error-test gdbm-test

all: $(PROGS)

$(PROGS): %: %.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)
