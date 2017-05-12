kernel/boot/izixboot_main.o: \
		kernel/include/tty/tty_driver.h \
		kernel/include/tty/tty_vga_text.h \
		kernel/include/kprint/kprint.h \
		kernel/include/mm/freemem.h \
		kernel/arch/$(ARCH)/include/izixboot/memmap.h \
		kernel/arch/$(ARCH)/include/izixboot/gdt.h
ifeq (x86,$(ARCH))
kernel/boot/izixboot_main.o: \
		kernel/arch/$(ARCH)/include/mm/gdt.h \
		kernel/arch/$(ARCH)/include/mm/e820.h \
		kernel/arch/$(ARCH)/include/mm/paging.h \
		kernel/arch/$(ARCH)/include/sched/tss.h \
		kernel/arch/$(ARCH)/include/int/idt.h \
		kernel/arch/$(ARCH)/include/irq/irq_vectors.h \
		kernel/arch/$(ARCH)/include/isr/isr.h \
		kernel/arch/$(ARCH)/include/pic_8259/pic_8259.h
endif
