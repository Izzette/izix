kernel/dev/dev.o: \
		libk/include/attributes.h \
		libk/include/collections/bintree.h \
		kernel/include/mm/malloc.h \
		kernel/include/kprint/kprint.h \
		kernel/include/kpanic/kpanic.h \
		kernel/include/dev/dev_types.h \
		kernel/include/dev/dev_driver.h \
		kernel/arch/$(ARCH)/include/mm/page.h \
		kernel/arch/$(ARCH)/include/mm/paging.h
