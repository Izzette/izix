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
objects_boot := kernel/arch/$(ARCH)/boot/$(BOOTLOADER)_main.o

# Our custom .init and .fini sections.
objects_source_crt := crti.o crtn.o
ifeq (izixboot,$(BOOTLOADER))
objects_source_crt := $(objects_source_crt) crti_zero_bss.o
endif
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

ifeq (x86,$(ARCH))
objects_drivers_x86_pic_8259 := pic_8259.o
objects_drivers_x86_pic_8259 := \
	$(addprefix kernel/arch/$(ARCH)/drivers/pic_8259/,$(objects_drivers_x86_pic_8259))
objects_drivers := $(objects_drivers) $(objects_drivers_x86_pic_8259)
endif

# Our kprint subsystem.
objects_kprint := kprint.o
objects_kprint := $(addprefix kernel/kprint/,$(objects_kprint))

objects_kpanic := kpanic.o
objects_kpanic := $(addprefix kernel/kpanic/,$(objects_kpanic))

objects_mm := freemem.o malloc.o
objects_mm := $(addprefix kernel/mm/,$(objects_mm))

ifeq (x86,$(ARCH))
objects_x86_mm := gdt.o e820.o paging.o
objects_x86_mm := $(addprefix kernel/arch/$(ARCH)/mm/,$(objects_x86_mm))
objects_mm := $(objects_mm) $(objects_x86_mm)
endif

objects_sched := spinlock.o mutex.o kthread.o
objects_sched := $(addprefix kernel/sched/,$(objects_sched))

objects_sched_asm :=
objects_sched_asm := $(addprefix kernel/sched/,$(objects_sched_asm))

ifeq (x86,$(ARCH))
objects_x86_sched := tss.o kthread_task.o kthread_preempt.o
objects_x86_sched := $(addprefix kernel/arch/$(ARCH)/sched/,$(objects_x86_sched))
objects_sched := $(objects_sched) $(objects_x86_sched)
objects_x86_sched_asm := kthread_switch.o kthread_bootstrap.o
objects_x86_sched_asm := $(addprefix kernel/arch/$(ARCH)/sched/,$(objects_x86_sched_asm))
objects_sched_asm := $(objects_sched_asm) $(objects_x86_sched_asm)
endif

objects_int :=
objects_int := $(addprefix kernel/int/,$(objects_int))

ifeq (x86,$(ARCH))
objects_x86_int := idt.o int_selector_ec.o
objects_x86_int := $(addprefix kernel/arch/$(ARCH)/int/,$(objects_x86_int))
objects_int := $(objects_int) $(objects_x86_int)
endif

objects_isr :=
objects_isr := $(addprefix kernel/isr/,$(objects_isr))

ifeq (x86,$(ARCH))
objects_x86_isr := df.o np.o gp.o irq.o
objects_x86_isr := $(addprefix kernel/arch/$(ARCH)/isr/,$(objects_x86_isr))
objects_isr := $(objects_isr) $(objects_x86_isr)
endif

objects_irq :=
objects_irq := $(addprefix kernel/irq/,$(objects_irq))

ifeq (x86,$(ARCH))
objects_x86_irq := irq.o
objects_x86_irq := $(addprefix kernel/arch/$(ARCH)/irq/,$(objects_x86_irq))
objects_irq := $(objects_irq) $(objects_x86_irq)
endif

# libk string objects
objects_libk_string := memchr.o memcpy.o memset.o strcat.o strlen.o
objects_libk_string := $(addprefix libk/string/,$(objects_libk_string))

# libk strings objects
objects_libk_strings := ffs.o
objects_libk_strings := $(addprefix libk/strings/,$(objects_libk_strings))

# libk format objects
objects_libk_format := pad.o itoa.o sprintf.o
objects_libk_format := $(addprefix libk/format/,$(objects_libk_format))

objects_libk_collections := bintree.o linked_list.o sparse_collection.o
objects_libk_collections := $(addprefix libk/collections/,$(objects_libk_collections))

# All Libk format objects
objects_libk := \
	$(objects_libk_string) \
	$(objects_libk_strings) \
	$(objects_libk_format) \
	$(objects_libk_collections)

# Libk library
libk := libk.a
libk := $(addprefix libk/,$(libk))

# Objects based on build tasks.
asm_source_objects := \
	$(object_start) \
	$(objects_source_crt) \
	$(objects_isr) \
	$(objects_sched_asm)
c_source_objects := \
	$(objects_boot) \
	$(objects_drivers) \
	$(objects_kprint) \
	$(objects_kpanic) \
	$(objects_mm) \
	$(objects_sched) \
	$(objects_int) \
	$(objects_irq) \
	$(objects_libk)

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
objects_begin_crt := crti.o
ifeq (izixboot,$(BOOTLOADER))
objects_begin_crt := $(objects_begin_crt) crti_zero_bss.o
endif
objects_begin_crt := $(objects_begin_crt) crtbegin.o
objects_begin_crt := $(addprefix kernel/arch/$(ARCH)/crt/,$(objects_begin_crt))

# Kernel objects.
objects_kernel := \
	$(objects_boot) \
	$(objects_drivers) \
	$(objects_mm) \
	$(objects_sched) \
	$(objects_sched_asm) \
	$(objects_int) \
	$(objects_isr) \
	$(objects_irq) \
	$(objects_kprint) \
	$(objects_kpanic)

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
	$(CFLAGS) \
	-I./kernel/include \
	-I./kernel/arch/$(ARCH)/include \
	-I./libk/include \
	-ffreestanding \
	-Werror=format \
	$(addprefix -DARCH_,$(shell echo $(ARCH) | tr a-z A-Z))

# Our assembling flags.
ASFLAGS ?= \
	-Wall -Wextra

# Our linker flags.
LDFLAGS := \
	-L./libk \
	-Wl,-T./$(linker_script)

STRIPFLAGS ?= \
	--remove-section=.note.gnu.gold-version \
	--remove-section=.comment \
	--remove-section=.note \
	--remove-section=.note.gnu.build-id \
	--remove-section=.note.ABI-tag \
	--strip-all \
	--strip-unneeded


####################
# Start of targets #
####################

# Our default target must go first.
.PHONY: all
all: izix.kernel

# All our included dependancies.
include $(wildcard $(addsuffix /*.d,$(all_object_dirs)))

$(asm_source_objects):%.o:%.s
	$(CC) $(ASFLAGS) -Wa,-I./$(dir $<) -c $< -o $@

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
