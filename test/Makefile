ROOT = ..
include $(ROOT)/Make.defines
EXTRALIBS = -lgdbm

CPPFLAGS = -I$(LDDIR) -I$(ROOT)

PROGS = error-test gdbm-test

all: $(PROGS)

$(PROGS): %: %.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)
