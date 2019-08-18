ROOT = ..
include $(ROOT)/Make.defines

CPPFLAGS = -I$(LDDIR)

PROGS = error-test

all: $(PROGS)

$(PROGS): %: %.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)
