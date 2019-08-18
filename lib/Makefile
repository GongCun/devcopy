ROOT = ..
include $(ROOT)/Make.defines

OBJS1 = list.o
OBJS2 = error.o

all: $(LIBMYADT) $(LIBMYERR)

$(LIBMYADT): $(OBJS1)
	$(AR) rcs $(LIBMYADT) $?

$(LIBMYERR): $(OBJS2)
	$(AR) rcs $(LIBMYERR) $?
