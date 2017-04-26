# libk/Makefile

CC ?= gcc
AR ?= ar
CFLAGS ?= -O2 -Wall -Wextra
CFLAGS := $(CFLAGS) -m32 \
          -I./include \
          -ffreestanding -nostdlib

objects_string := memchr.o memcpy.o strcat.o strlen.o
objects_string := $(addprefix string/,$(objects_string))

objects := $(objects_string)

all: libk.a

include $(wildcard string/*.d)

$(objects):%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

libk.a: $(objects)
	$(AR) rcs libk.a $(objects)

clean: clean_libk clean_string

clean_libk:
	rm -f libk.a

clean_string:
	rm -f string/*.o

# vim: set ts=4 sw=4 noet syn=make:
