INCLUDEDIR = /root/arm/kernel/linux/include

CC = arm-linux-gcc
LD = arm-linux-ld

CFLAGS = -D__KERNEL__ -DMODULE -O2 -Wall -I$(INCLUDEDIR) --save-temps

VER = $(shell awk -F\" '/REL/ {print $$2}' $(INCLUDEDIR)/linux/version.h)

OBJS = flash.o

all: $(OBJS)

flash.o: module.o flash_intel.o
	$(LD) -r $^ -o $@

install: $(OBJS)
	cp $(OBJS) /usr/exports/usr/local/bin

clean:
	rm -f *.o *~ core
