kernel/boot/izixboot_main.o: \
		izixboot/include/izixboot/memmap.h izixboot/include/izixboot/gdt.h izixboot/include/izixboot/gdt32.h \
		libk/include/string.h libk/include/format.h \
		kernel/arch/$(ARCH)/include/asm/io.h \
		kernel/include/video/vga_text.h
