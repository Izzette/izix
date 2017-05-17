kernel/sched/mutex.o: \
		libk/include/collections/linked_list.h \
		kernel/include/mm/malloc.h \
		kernel/include/sched/mutex.h \
		kernel/include/kprint/kprint.h \
		kernel/include/kpanic/kpanic.h \
		kernel/arch/$(ARCH)/include/sched/spinlock.h \
		kernel/arch/$(ARCH)/include/sched/kthread.h
