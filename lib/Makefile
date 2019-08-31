ROOT = ..
EXTRA := -fPIC -D_GNU_SOURCE
include $(ROOT)/Make.defines

OBJS = ktree.o list.o error.o

all: $(LIBMYLIB)

$(LIBMYLIB): $(OBJS)
	$(AR) rv $(LIBMYLIB) $?
	$(RANLIB) $(LIBMYLIB)

