ROOT = ..
OS = $(shell uname -s | tr 'A-Z' 'a-z')
# EXTRA := -fPIC -D_GNU_SOURCE
EXTRA := -fPIC
include $(ROOT)/Make.defines.$(OS)

OBJS = ktree.o list.o error.o asprintf.o lockfile.o

all: $(LIBMYLIB)

$(LIBMYLIB): $(OBJS)
	$(AR) rv $(LIBMYLIB) $?
	$(RANLIB) $(LIBMYLIB)

clean:
	rm -f $(TEMPFILES) $(LIBMYLIB)

