INCLUDEDIR = /root/arm/kernel/linux/include

CC = arm-linux-gcc
LD = arm-linux-ld

CFLAGS = -D__KERNEL__ -DMODULE -O2 -Wall -I$(INCLUDEDIR)

VER = $(shell awk -F\" '/REL/ {print $$2}' $(INCLUDEDIR)/linux/version.h)

OBJS = keypad.o

all: $(OBJS)

keypad.o: module.o interrupt.o keyproc.o
	$(LD) -r $^ -o $@

install: $(OBJS)
	cp $(OBJS) /usr/exports/usr/local/bin

clean:
	rm -f *.o *~ core
