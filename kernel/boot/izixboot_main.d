kernel/boot/izixboot_main.o: \
		izixboot/include/izixboot/memmap.h izixboot/include/izixboot/gdt.h \
		kernel/include/tty/tty_driver.h kernel/include/tty/tty_vga_text.h \
		kernel/include/kprint/kprint.h \
		kernel/include/mm/freemem.h \
		kernel/arch/x86/include/mm/gdt.h kernel/arch/x86/include/mm/e820.h \
		kernel/arch/x86/include/sched/tss.h \
		kernel/arch/x86/include/int/idt.h \
		kernel/arch/x86/include/irq/irq_vectors.h \
		kernel/arch/x86/include/isr/isr.h \
		kernel/arch/x86/include/pic_8259/pic_8259.h
