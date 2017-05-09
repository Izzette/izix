# Makefile

##############################
# Default config definitions #
##############################

ARCH ?= x86
BOOTLOADER ?= izixboot


#####################
# Utility functions #
#####################

define uniq =
  $(eval seen :=)
  $(foreach _,$1,$(if $(filter $_,${seen}),,$(eval seen += $_)))
  ${seen}
endef


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
objects_drivers_video := vga_text.o vga_cursor.o
objects_drivers_video := $(addprefix kernel/drivers/video/,$(objects_drivers_video))

# Our TTY drivers.
objects_drivers_tty := tty_driver.o tty_vga_text.o
objects_drivers_tty := $(addprefix kernel/drivers/tty/,$(objects_drivers_tty))

# All of our drivers.
objects_drivers := $(objects_drivers_video) $(objects_drivers_tty)

# Our kprint subsystem.
objects_kprint := kprint.o
objects_kprint := $(addprefix kernel/kprint/,$(objects_kprint))

objects_mm := freemem.o malloc.o
objects_mm := $(addprefix kernel/mm/,$(objects_mm))

ifeq (x86,$(ARCH))
objects_x86_mm := gdt.o e820.o
objects_x86_mm := $(addprefix kernel/arch/$(ARCH)/mm/,$(objects_x86_mm))
objects_mm := $(objects_mm) $(objects_x86_mm)
endif

objects_sched :=
objects_sched := $(addprefix kernel/sched/,$(objects_sched))

ifeq (x86,$(ARCH))
objects_x86_sched := tss.o
objects_x86_sched := $(addprefix kernel/arch/$(ARCH)/sched/,$(objects_x86_sched))
objects_sched := $(objects_sched) $(objects_x86_sched)
endif

# libk string objects
objects_libk_string := memchr.o memcpy.o memset.o strcat.o strlen.o
objects_libk_string := $(addprefix libk/string/,$(objects_libk_string))

# libk format objects
objects_libk_format := pad.o itoa.o sprintf.o
objects_libk_format := $(addprefix libk/format/,$(objects_libk_format))

objects_libk_collections := bintree.o
objects_libk_collections := $(addprefix libk/collections/,$(objects_libk_collections))

# All Libk format objects
objects_libk := $(objects_libk_string) $(objects_libk_format) $(objects_libk_collections)

# Libk library
libk := libk.a
libk := $(addprefix libk/,$(libk))

# Objects based on build tasks.
asm_source_objects := $(object_start) $(objects_source_crt)
c_source_objects := $(object_main) $(objects_drivers) $(objects_kprint) $(objects_mm) $(objects_sched) $(objects_libk)

# Object dirs
all_objects := $(objects_bin_crt) $(asm_source_objects) $(c_source_objects)
all_object_dirs := $(call uniq,$(realpath $(dir $(all_objects))))
all_object_dirs := $(all_object_dirs:$(CURDIR)/%=%)

# Clean targets
CLEAN_object_dirs := $(addprefix clean_,$(all_object_dirs))

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
objects_kernel := $(object_main) $(objects_drivers) $(objects_mm) $(objects_sched) $(objects_kprint)

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
STRIP ?= strip
OBJCOPY ?= objcopy
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
	-ffreestanding \
	-Werror=format

# Our assembling flags.
ASFLAGS ?= \
	-Wall -Wextra
ASFLAGS := \
	$(ASFLAGS) -DIZIX \
	-Wa,-I./kernel/arch/$(ARCH)/include

# Our linker flags.
LDFLAGS := \
	-L./libk \
	-Wl,-T$(linker_script)

STRIPFLAGS ?= \
	--remove-section=.note.gnu.gold-version \
	--remove-section=.comment \
	--remove-section=.note \
	--remove-section=.note.gnu.build-id \
	--remove-section=.note.ABI-tag
STRIPFLAGS := \
	--strip-all \
	--strip-unneeded \
	$(STRIPFLAGS)

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
.PHONY: all
all: izix.kernel

# All our included dependancies.
include $(wildcard kernel/arch/$(ARCH)/boot/*.d)
include $(wildcard kernel/arch/$(ARCH)/crt/*.d)
include $(wildcard kernel/arch/$(ARCH)/mm/*.d)
include $(wildcard kernel/boot/*.d)
include $(wildcard kernel/drivers/video/*.d)
include $(wildcard kernel/drivers/tty/*.d)
include $(wildcard kernel/kprint/*.d)

$(asm_source_objects):%.o:%.s
	$(CC) $(ASFLAGS) -c $< -o $@

$(c_source_objects):%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(objects_bin_crt):
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(shell basename $@)` && cp "$$OBJ" $@

$(libk): $(objects_libk)
	$(AR) rcs $@ $(objects_libk)

izix.kernel: $(linker_script) $(object_start) $(objects_source_crt) $(objects_bin_crt) $(objects_kernel) $(libk)
	$(CC) $(CFLAGS) $(LDFLAGS) \
		$(link_order) \
		-o $@

.PHONY: debug
debug: izix.debug

izix.debug: izix.kernel
	$(OBJCOPY) --only-keep-debug \
		izix.kernel izix.debug

.PHONY: clean
clean: clean_object_dirs clean_libk clean_izix.kernel clean_izix.debug

.PHONY: clean_object_dirs
clean_object_dirs: $(CLEAN_object_dirs)

.PHONY: $(CLEAN_object_dirs)
$(CLEAN_object_dirs):clean_%:%
	rm -f $(addsuffix /*.o,$<)

.PHONY: clean_libk
clean_libk:
	rm -f $(libk)

.PHONY: clean_izix.kernel
clean_izix.kernel:
	rm -f izix.kernel

.PHONY: clean_izix.debug
clean_izix.debug:
	rm -f izix.debug

.PHONY: strip
strip:
	$(STRIP) $(STRIPFLAGS) \
		izix.kernel

# vim: set ts=4 sw=4 noet syn=make:
