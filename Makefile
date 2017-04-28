# Makefile

##############################
# Default config definitions #
##############################

ARCH ?= x86
BOOTLOADER ?= izixboot


#############################
# Variables generate tagets #
#############################

# Earlier boot objects
object_start := kernel/arch/$(ARCH)/boot/$(BOOTLOADER)_start.o
object_main := kernel/boot/$(BOOTLOADER)_main.o

# Our custom .init and .fini sections.
objects_source_crt := crti.o crtn.o
objects_source_crt := $(addprefix kernel/arch/$(ARCH)/crt/,$(objects_source_crt))

# GCCs .init and .fini sections.
objects_bin_crt := crtbegin.o crtend.o
objects_bin_crt := $(addprefix kernel/arch/$(ARCH)/crt/,$(objects_bin_crt))

# Our video drivers.
objects_drivers_video := vga_text.o
objects_drivers_video := $(addprefix kernel/drivers/video/,$(objects_drivers_video))

# Our TTY drivers.
objects_drivers_tty := tty_vga_text.o
objects_drivers_tty := $(addprefix kernel/drivers/tty/,$(objects_drivers_tty))

# All of our drivers.
objects_drivers := $(objects_drivers_video) $(objects_drivers_tty)

# Our kprint subsystem.
objects_kprint := kprint.o
objects_kprint := $(addprefix kernel/kprint/,$(objects_kprint))

# Objects based on build tasks.
asm_source_objects := $(object_start) $(objects_source_crt)
c_source_objects := $(object_main) $(objects_drivers) $(objects_kprint)

# Our linker script
linker_script := $(BOOTLOADER).ld
linker_script := $(addprefix lds/,$(linker_script))


#########################
# Variables for linking #
#########################

# The very first object.
objects_first := $(object_start)

# .init section
objects_begin_crt := crti.o crtbegin.o
objects_begin_crt := $(addprefix kernel/arch/$(ARCH)/crt/,$(objects_begin_crt))

# Kernel objects.
objects_kernel := $(object_main) $(objects_drivers) $(objects_kprint)

# Libraries to link to.
libs := \
	-nostdlib \
	-lk \
	-lgcc

# .fini section
objects_end_crt := crtend.o crtn.o
objects_end_crt := $(addprefix kernel/arch/$(ARCH)/crt/,$(objects_end_crt))

# The link order.
link_order := \
	$(objects_first) \
	$(objects_begin_crt) \
	$(objects_kernel) \
	$(libs) \
	$(objects_end_crt)


#######################
# Toolchain variables #
#######################

# Our toolchain binaries.
CC ?= gcc
AR ?= ar
# We will use $(CC) for linking and assembling.
# LD ?=
# AS ?=

# Our C compiler flags.
CFLAGS ?= \
	-O2 -Wall -Wextra
CFLAGS := \
	$(CFLAGS) -DIZIX \
	-I./kernel/include \
	-I./kernel/arch/$(ARCH)/include \
	-I./libk/include \
	-ffreestanding

# Our assembling flags.
ASFLAGS ?= \
	-Wall -Wextra
ASFLAGS := \
	$(ASFLAGS) -DIZIX \
	-Wa,-I./kernel/arch/$(ARCH)/include \
	-fno-zero-initialized-in-bss \

# Our linker flags.
LDFLAGS := \
	-L./libk \
	-Wl,-T$(linker_script)

# Bootloader specific flags.
ifeq (izixboot,$(BOOTLOADER))
CFLAGS := \
	$(CFLAGS) \
	-fno-zero-initialized-in-bss
ASFLAGS := \
	$(ASFLAGS) \
	-fno-zero-initialized-in-bss
endif


####################
# Start of targets #
####################

# Our default target must go first.
all: izix.kernel

# All our included dependancies.
include $(wildcard kernel/arch/$(ARCH)/boot/*.d)
include $(wildcard kernel/arch/$(ARCH)/crt/*.d)
include $(wildcard kernel/boot/*.d)
include $(wildcard kernel/drivers/video/*.d)
include $(wildcard kernel/drivers/tty/*.d)
include $(wildcard kernel/kprint/*.d)

.PHONY: libk_subsystem
libk_subsystem:
	$(MAKE) -C libk

$(asm_source_objects):%.o:%.s
	$(CC) $(ASFLAGS) -c $< -o $@

$(c_source_objects):%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(objects_bin_crt):
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(shell basename $@)` && cp "$$OBJ" $@

izix.kernel: $(linker_script) $(object_start) $(objects_source_crt) $(objects_bin_crt) $(objects_kernel) libk_subsystem
	$(CC) $(CFLAGS) $(LDFLAGS) \
		$(link_order) \
		-o $@

.PHONY: clean
clean: clean_libk clean_x86_boot clean_x86_crt clean_boot clean_drivers clean_kprint clean_kernel

.PHONY: clean_libk
clean_libk:
	$(MAKE) -C libk clean

.PHONY: clean_x86_boot
clean_x86_boot:
	rm -f kernel/arch/$(ARCH)/boot/*.o

.PHONY: clean_x86_crt
clean_x86_crt:
	rm -f kernel/arch/$(ARCH)/crt/*.o

.PHONY: clean_boot
clean_boot:
	rm -f kernel/boot/*.o

.PHONY: clean_drivers
clean_drivers: clean_drivers_video clean_drivers_tty

.PHONY: clean_drivers_video
clean_drivers_video:
	rm -f kernel/drivers/video/*.o

.PHONY: clean_drivers_tty
clean_drivers_tty:
	rm -f kernel/drivers/tty/*.o

.PHONY: clean_kprint
clean_kprint:
	rm -f kernel/kprint/*.o

.PHONY: clean_kernel
clean_kernel:
	rm -f izix.kernel

# vim: set ts=4 sw=4 noet syn=make:
