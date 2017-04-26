# libk/Makefile

CC ?= gcc
AR ?= ar
CFLAGS ?= -O2 -Wall -Wextra
CFLAGS := $(CFLAGS) -m32 \
          -I./include \
          -ffreestanding -nostdlib

objects_string := memchr.o memcpy.o strcat.o strlen.o
objects_string := $(addprefix string/,$(objects_string))

objects_format := pad.o itoa.o
objects_format := $(addprefix format/,$(objects_format))

objects := $(objects_string) $(objects_format)

all: libk.a

include $(wildcard string/*.d)
include $(wildcard format/*.d)

$(objects):%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

libk.a: $(objects)
	$(AR) rcs libk.a $(objects)

clean: clean_libk clean_string clean_format

clean_libk:
	rm -f libk.a

clean_string:
	rm -f string/*.o

clean_format:
	rm -f format/*.o

# vim: set ts=4 sw=4 noet syn=make:
