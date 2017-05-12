ifeq (x86,$(ARCH))
kernel/boot/main_loop.o: \
		kernel/arch/$(ARCH)/include/asm/toggle_int.h \
		kernel/arch/$(ARCH)/include/asm/halt.h
endif
