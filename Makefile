ROOT = .
OS = $(shell uname -s | tr 'A-Z' 'a-z')
include $(ROOT)/Make.defines.$(OS)
#EXTRALIBS = -lgdbm_compat -lgdbm -lz -lncurses

CPPFLAGS = -I$(LDDIR) -I$(ROOT)

PROGS = devcopy devcopy-hash devcopy-delta devcopy-version

all:	$(PROGS)

$(PROGS): %: %.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(TEMPFILES) $(PROGS)
