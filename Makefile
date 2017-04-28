# Makefile

ARCH ?= x86
BOOTLOADER ?= izixboot

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
CFLAGS := $(CFLAGS) -m32 -DIZIX \
          -I./kernel/include \
          -I./kernel/arch/$(ARCH)/include \
          -I./libk/include \
          -fno-zero-initialized-in-bss \
          -ffreestanding -nostdlib

object_start := kernel/arch/$(ARCH)/boot/$(BOOTLOADER)_start.o
object_main := kernel/boot/$(BOOTLOADER)_main.o

objects_crt := crti.o crtn.o
objects_crt := $(addprefix kernel/arch/$(ARCH)/crt/,$(objects_crt))

objects_drivers_video := vga_text.o
objects_drivers_video := $(addprefix kernel/drivers/video/,$(objects_drivers_video))

objects_drivers_tty := tty_vga_text.o
objects_drivers_tty := $(addprefix kernel/drivers/tty/,$(objects_drivers_tty))

objects_drivers := $(objects_drivers_video) $(objects_drivers_tty)

objects_kprint := kprint.o
objects_kprint := $(addprefix kernel/kprint/,$(objects_kprint))

asm_source_objects := $(object_start)
pp_asm_source_objects := $(objects_crt)
c_source_objects := $(object_main) $(objects_drivers) $(objects_kprint)

.PHONY: \
	libk_subsystem \
	clean clean_libk clean_x86_boot clean_x86_crt \
	clean_boot \
	clean_drivers_video clean_drivers_tty clean_drivers \
	clean_kprint \
	clean_kernel

all: izix.kernel

include $(wildcard kernel/arch/$(ARCH)/boot/*.d)
include $(wildcard kernel/arch/$(ARCH)/crt/*.d)
include $(wildcard kernel/boot/*.d)
include $(wildcard kernel/drivers/video/*.d)
include $(wildcard kernel/drivers/tty/*.d)
include $(wildcard kernel/kprint/*.d)

boot: $(object_start) $(object_main)

libk_subsystem:
	$(MAKE) -C libk

lib/libk.a: libk_subsystem

$(asm_source_objects):%.o:%.s
	$(CC) $(CFLAGS) -c $< -o $@

$(pp_asm_source_objects):%.o:%.S
	$(CC) $(CFLAGS) -c $< -o $@

$(c_source_objects):%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

izix.kernel: lds/linker.ld $(object_start) $(objects_crt) $(object_main) $(objects_drivers) $(objects_kprint) lib/libk.a
	$(CC) $(CFLAGS) -Wl,-T,lds/linker.ld \
		$(object_start) $(objects_crt) $(object_main) $(objects_drivers) $(objects_kprint) \
		libk/libk.a -lgcc \
		-o izix.kernel

clean: clean_libk clean_x86_boot clean_x86_crt clean_boot clean_drivers clean_kprint clean_kernel

clean_libk:
	$(MAKE) -C libk clean

clean_x86_boot:
	rm -f kernel/arch/$(ARCH)/boot/*.o

clean_x86_crt:
	rm -f kernel/arch/$(ARCH)/crt/*.o

clean_boot:
	rm -f kernel/boot/*.o

clean_drivers: clean_drivers_video clean_drivers_tty

clean_drivers_video:
	rm -f kernel/drivers/video/*.o

clean_drivers_tty:
	rm -f kernel/drivers/tty/*.o

clean_kprint:
	rm -f kernel/kprint/*.o

clean_kernel:
	rm -f izix.kernel

# vim: set ts=4 sw=4 noet syn=make:
