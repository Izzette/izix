# Makefile

ARCH ?= x86

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
CFLAGS := $(CFLAGS) -m32 -DIZIX \
          -I$(shell pwd)/arch/$(ARCH)/include \
          -ffreestanding -nostdlib

all: izix.kernel

boot.s:
crti.S:
crtn.S:
kernel.c:
linker.ld:

boot.o: boot.s
	$(CC) $(CFLAGS) -c boot.s -o boot.o

crti.o: crti.S
	$(CC) $(CFLAGS) -c crti.S -o crti.o

crtn.o: crtn.S
	$(CC) $(CFLAGS) -c crtn.S -o crtn.o

kernel.o: arch/$(ARCH)/include/asm/io.h kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

izix.kernel: linker.ld boot.o crti.o crtn.o kernel.o
	$(CC) $(CFLAGS) -T linker.ld -o izix.kernel boot.o crti.o crtn.o kernel.o -lgcc

clean:
	rm -f *.o izix.kernel

# vim: set ts=4 sw=4 noet syn=make:
