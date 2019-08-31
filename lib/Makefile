ROOT = ..
# EXTRA := -fPIC -D_GNU_SOURCE
EXTRA := -fPIC
include $(ROOT)/Make.defines

OBJS = ktree.o list.o error.o asprintf.o

all: $(LIBMYLIB)

$(LIBMYLIB): $(OBJS)
	$(AR) rv $(LIBMYLIB) $?
	$(RANLIB) $(LIBMYLIB)

