kernel/sched/kthread.o: \
		libk/include/collections/linked_list.h \
		libk/include/collections/bintree.h \
		kernel/include/mm/malloc.h \
		kernel/include/kprint/kprint.h \
		kernel/include/kpanic/kpanic.h \
		kernel/include/sched/kthread_kpid.h \
		kernel/arch/$(ARCH)/include/asm/halt.h \
		kernel/arch/$(ARCH)/include/sched/spinlock.h \
		kernel/arch/$(ARCH)/include/sched/kthread_task.h \
		kernel/arch/$(ARCH)/include/sched/kthread_preempt.h
