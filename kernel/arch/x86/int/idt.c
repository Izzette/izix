// kernel/arch/x86/int/idt.c

#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <mm/malloc.h>
#include <mm/gdt.h>
#include <int/idt.h>

static idt_register_t *idt_registry;

void idt_init () {
	size_t i;
	idt_entry_t *entries;
	idt_register_logical_t logical_registry;

	entries = malloc (IDT_NUMBER_OF_INTERUPTS * sizeof(idt_entry_t));
	if (!entries) {
		kputs ("int/idt: Failed to allocate the IDT!\n");
		kpanic ();
	}

	for (i = 0; IDT_NUMBER_OF_INTERUPTS > i; ++i) {
		idt_entry_logical_t logical_entry = {
			.isr_offset = NULL,
			.selector = gdt_get_first_code_selector (),
			.type = IDT_I386_INTERUPT_GATE,
			.storage = false,
			.priviledge = 0,
			.present = false
		};

		entries[i] = idt_entry_encode (logical_entry);
	}

	logical_registry = (idt_register_logical_t){
		.limit = IDT_NUMBER_OF_INTERUPTS,
		.base = entries
	};

	idt_registry = malloc (sizeof(idt_register_t));
	if (!idt_registry) {
		kputs ("int/idt: Failed to allocate the IDT registry!\n");
		kpanic ();
	}

	*idt_registry = idt_register_encode (logical_registry);
}

void idt_set_isr (interupt_vector_t vector, void (*isr) ()) {
	idt_register_logical_t logical_registry;
	idt_entry_logical_t logical_entry;

	logical_registry = idt_register_decode (*idt_registry);
	logical_entry = idt_entry_decode (logical_registry.base[vector]);

	logical_entry.present = true;
	logical_entry.isr_offset = isr;

	logical_registry.base[vector] = idt_entry_encode (logical_entry);
}

void idt_load () {
	asm volatile (
		"		lidt		(%0);\n"
		:
		:"r"(idt_registry));

	kputs ("int/idt: IDT loaded successfuly.\n");
}

// vim: set ts=4 sw=4 noet syn=c:
