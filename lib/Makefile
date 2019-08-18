ROOT = ..
EXTRA := -fPIC
include $(ROOT)/Make.defines

OBJS = list.o error.o

all: $(LIBMYLIB)

$(LIBMYLIB): $(OBJS)
	$(AR) rv $(LIBMYLIB) $?
	$(RANLIB) $(LIBMYLIB)

