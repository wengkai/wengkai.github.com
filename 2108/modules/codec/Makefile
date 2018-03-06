INCLUDEDIR = /root/arm/kernel/linux/include

CC = arm-linux-gcc
LD = arm-linux-ld

CFLAGS = -D__KERNEL__ -DMODULE -O2 -Wall -I$(INCLUDEDIR)

VER = $(shell awk -F\" '/REL/ {print $$2}' $(INCLUDEDIR)/linux/version.h)

OBJS = codec.o

all: $(OBJS)

codec.o: module.o
	$(LD) -r $^ -o $@

install: $(OBJS)
	cp $(OBJS) /usr/exports/usr/local

clean:
	rm -f *.o *~ core
