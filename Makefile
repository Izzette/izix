# Makefile

ARCH ?= x86

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
CFLAGS := $(CFLAGS) -m32 -DIZIX \
          -I$(shell pwd)/arch/$(ARCH)/include \
          -ffreestanding -nostdlib -lgcc

all: myos.bin

boot.s:
kernel.c:
linker.ld:

boot.o: boot.s
	$(CC) $(CFLAGS) -c boot.s -o boot.o

kernel.o: arch/$(ARCH)/include/asm/io.h kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

myos.bin: linker.ld boot.o kernel.o
	$(CC) $(CFLAGS) -T linker.ld -o myos.bin boot.o kernel.o

clean:
	rm -f *.o myos.bin

# vim: set ts=4 sw=4 noet syn=make:
