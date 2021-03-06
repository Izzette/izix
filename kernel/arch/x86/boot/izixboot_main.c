// kernel/arch/x86/boot/izixboot_main.c

#include <stddef.h>
#include <stdint.h>

#include <attributes.h>

#include <dev/dev.h>
#include <tty/tty_chardev_driver.h>
#include <tty/tty_vga_text.h>
#include <kprint/kprint.h>
#include <asm/toggle_int.h>
#include <mm/malloc.h>
#include <mm/freemem.h>
#include <mm/gdt.h>
#include <mm/e820.h>
#include <mm/paging.h>
#include <sched/tss.h>
#include <sched/kthread.h>
#include <int/idt.h>
#include <irq/irq_vectors.h>
#include <isr/isr.h>
#include <pic_8259/pic_8259.h>
#include <pit_8253/pit_8253.h>
#include <cmos/rtc.h>
#include <time/time.h>
#include <time/clock.h>
#include <time/clock_tick.h>

#define KERNEL_START      ((void *)0x8000)
#define KERNEL_MAX_LENGTH (127 * (size_t)512)
#define KERNEL_MAX_END    (KERNEL_START + KERNEL_MAX_LENGTH)

#if !defined(IZIX)
#define AMERICAN_DATE
#endif

static volatile int v = 0;
static volatile int n = 0;

void show_date () {
	rtc_datetime_t datetime = rtc_get_datetime ();

	kprintf (
		"boot/izixboot_main: The current date and time is "
		"%02hhd/%02hhd/%02hhd%02hhd %02hhd:%02hhd:%02hhd.\n",
#ifdef AMERICAL_DATE
		datetime.month,
		datetime.monthday,
#else
		datetime.monthday,
		datetime.month,
#endif
		datetime.century,
		datetime.year,
		datetime.hours,
		datetime.minutes,
		datetime.seconds);
}

COLD FORCE_ALIGN_ARG_POINTER
void kernel_main (
		uint32_t stack_start_u32,
		uint32_t stack_length_u32,
		uint32_t bss_end_u32,
		uint32_t e820_entry_count_u32,
		uint32_t e820_entries_3x_u32
) {
	size_t e820_entry_count = (size_t)e820_entry_count_u32;
	size_t stack_length = (size_t)stack_length_u32;
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
	void *stack_start = (void *)stack_start_u32;
	void *bss_end = (void *)bss_end_u32;
	void *e820_entries_3x = (void *)e820_entries_3x_u32;
#pragma GCC diagnostic pop

	const size_t kernel_and_bss_length =
		((bss_end > KERNEL_MAX_END) ?
			(size_t)(bss_end - KERNEL_START)
			: KERNEL_MAX_LENGTH);

	freemem_region_t
		kernel_region, null_region, stack_region,
		int_stack_region, freemem_internal_region,
		tty_chardev_driver_region;

	// TODO: Get the actually start and length of the kernel by parsing its own ELF at
	// runtime or by createing all the nessesary symbols in the linker script.
	kernel_region = new_freemem_region (
		KERNEL_START,
		kernel_and_bss_length);
	null_region = new_freemem_region (
		NULL,
		MALLOC_ALIGNMENT);
	stack_region = new_freemem_region (
		stack_start + MALLOC_ALIGNMENT,
		stack_length - MALLOC_ALIGNMENT);
	int_stack_region = new_freemem_region (
		freemem_region_end (kernel_region), // Exclusive max.
		KTHREAD_STACK_SIZE);
	freemem_internal_region = new_freemem_region (
		freemem_region_end (int_stack_region), // Exclusive max.
		9 * PAGE_SIZE);
	tty_chardev_driver_region = new_freemem_region (
		freemem_region_end (freemem_internal_region), // Exclusive max.
		sizeof(tty_chardev_driver_t));

	if (tty_chardev_driver_region.length % MALLOC_ALIGNMENT)
		tty_chardev_driver_region.length +=
			MALLOC_ALIGNMENT - sizeof(tty_chardev_driver_t) % MALLOC_ALIGNMENT;

	volatile tty_chardev_driver_t *tty_chardev_driver_vga_text =
		tty_chardev_driver_region.p;
	*tty_chardev_driver_vga_text = new_tty_vga_driver ();
	tty_chardev_driver_vga_text->init (
		(tty_chardev_driver_t *)tty_chardev_driver_vga_text);
	set_kprint_tty_chardev_driver (tty_chardev_driver_vga_text);

	kprintf (
		"boot/izixboot_main: Provided command line:\n"
			"\tstack_start=%p\n"
			"\tstack_length=0x%zx\n"
			"\tbss_end=%p\n"
			"\te820_entry_count=%zd\n"
			"\te820_entries_3x=%p\n",
		stack_start,
		stack_length,
		bss_end,
		e820_entry_count,
		e820_entries_3x);

	freemem_init (freemem_internal_region.p, freemem_internal_region.length);

	e820_3x_register (e820_entry_count, e820_entries_3x);
	e820_3x_print ();
	e820_3x_add_freemem ();

	freemem_remove_region (tty_chardev_driver_region);
	freemem_remove_region (freemem_internal_region);
	freemem_remove_region (int_stack_region);
	freemem_remove_region (kernel_region);
	freemem_remove_region (stack_region);
	freemem_remove_region (null_region);

	idt_init ();

	idt_set_isr (IDT_NP_VECTOR, isr_np);
	idt_set_isr (IDT_GP_VECTOR, isr_gp);
	idt_set_isr (IDT_DF_VECTOR, isr_df);

	idt_set_isr (IRQ_VECTOR_IRQ0, isr_irq0);
	// IRQ2 is used internally by 8259PIC.
	idt_set_isr (IRQ_VECTOR_IRQ1, isr_irq1);
	idt_set_isr (IRQ_VECTOR_IRQ3, isr_irq3);
	idt_set_isr (IRQ_VECTOR_IRQ4, isr_irq4);
	idt_set_isr (IRQ_VECTOR_IRQ5, isr_irq5);
	idt_set_isr (IRQ_VECTOR_IRQ6, isr_irq6);
	idt_set_isr (IRQ_VECTOR_IRQ7, isr_irq7);
	idt_set_isr (IRQ_VECTOR_IRQ8, isr_irq8);
	idt_set_isr (IRQ_VECTOR_IRQ9, isr_irq9);
	idt_set_isr (IRQ_VECTOR_IRQ10, isr_irq10);
	idt_set_isr (IRQ_VECTOR_IRQ11, isr_irq11);
	idt_set_isr (IRQ_VECTOR_IRQ12, isr_irq12);
	idt_set_isr (IRQ_VECTOR_IRQ13, isr_irq13);
	idt_set_isr (IRQ_VECTOR_IRQ14, isr_irq14);
	idt_set_isr (IRQ_VECTOR_IRQ15, isr_irq15);

	pic_8259_reinit ();

	irq_init ();
	idt_load ();
	enable_int ();

	// We want to get the date before we start the RTC interupts in clock_tick_start.
	show_date ();
	// Hooks need to be added to IRQ hook lists before the IDT is loaded.
	clock_tick_start ();

	tss_init (freemem_region_end (int_stack_region));
	// TSS must be initialized before tss_get () does any good.
	gdt_init (tss_get ());
	tss_load (GDT_SUPERVISOR_TSS_SELECTOR);

	paging_data_t paging_data_base;
	paging_init (&paging_data_base);

	dev_add (tty_chardev_driver_vga_text->dev_driver);
	dev_add (pic_8259_get_device_driver ());
	dev_add (pit_8253_get_device_driver ());
	dev_add (rtc_get_device_driver ());

	e820_3x_map_physical (&paging_data_base);
	dev_map_all (&paging_data_base);

	paging_enable (&paging_data_base);

	kthread_init (stack_region);

	kprintf (
		"boot/izixboot_main: Early boot took aprox. %lld ms.\n",
		time_millis (clock_get_boot_time ()));

	kthread_end_task ();
}

// vim: set ts=4 sw=4 noet syn=c:
