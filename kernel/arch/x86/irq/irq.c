// kernel/arch/x86/irq/irq.c

#include <collections/linked_list.h>

#include <asm/toggle_int.h>
#include <mm/malloc.h>
#include <kprint/kprint.h>
#include <kpanic/kpanic.h>
#include <irq/irq.h>
#include <pic_8259/pic_8259.h>

// Interupt handlers must be very fast, so we've cut out all the stops and optimized the
// important functions.

TPL_LINKED_LIST(irq_hook, irq_hook_t)

static volatile linked_list_irq_hook_t *irq_pre_hooks;
static volatile linked_list_irq_hook_t *irq_post_hooks;

__attribute__((optimize("O3")))
static volatile linked_list_irq_hook_t *irq_get_pre_hook_list (irq_t irq) {
	return &irq_pre_hooks[irq];
}

__attribute__((optimize("O3")))
static volatile linked_list_irq_hook_t *irq_get_post_hook_list (irq_t irq) {
	return &irq_post_hooks[irq];
}

static void irq_add_hook (
		volatile linked_list_irq_hook_t *hook_list,
		irq_hook_t hook
) {
	linked_list_irq_hook_node_t *hook_node =
		malloc (sizeof(linked_list_irq_hook_node_t));
	if (!hook_node) {
		kputs ("irq/irq: Failed to allocate IRQ hook node while adding a hook!\n");
		kpanic ();
	}

	*hook_node = new_linked_list_irq_hook_node (hook);

	hook_list->append (
		(linked_list_irq_hook_t *)hook_list,
		hook_node);
}

__attribute__((optimize("O3")))
static void irq_run_hooks (
		irq_t irq,
		volatile linked_list_irq_hook_t *hook_list
) {
	linked_list_irq_hook_iterator_t
		iterator_base,
		*iterator = &iterator_base;
	linked_list_irq_hook_node_t *hook_node;

	iterator_base = hook_list->new_iterator (
		(linked_list_irq_hook_t *)hook_list);

	hook_node = iterator->cur (iterator);
	while (hook_node) {
		hook_node->data (irq);
		hook_node = iterator->next (iterator);
	}
}

void irq_init () {
	irq_t irq;

	irq_pre_hooks = malloc (IRQ_NUMBER_OF_IRQ_LINES * sizeof(linked_list_irq_hook_t));
	irq_post_hooks = malloc (IRQ_NUMBER_OF_IRQ_LINES * sizeof(linked_list_irq_hook_t));
	if (!irq_pre_hooks || !irq_post_hooks) {
		kputs ("irq/irq: Failed to allocate IRQ hook lists!\n");
		kpanic ();
	}

	for (irq = 0; IRQ_NUMBER_OF_IRQ_LINES > irq; ++irq) {
		// If there are no handlers, then there is no point in recieving those interupts.
		pic_8259_mask (irq);

		*irq_get_pre_hook_list  (irq) = new_linked_list_irq_hook ();
		*irq_get_post_hook_list (irq) = new_linked_list_irq_hook ();
	}
}

void irq_add_pre_hook (irq_t irq, irq_hook_t hook) {
	pic_8259_mask (irq);

	irq_add_hook (irq_get_pre_hook_list (irq), hook);

	// This will unmask the interupt now that there is a pre hook to run.
	pic_8259_unmask (irq);
}

void irq_add_post_hook (irq_t irq, irq_hook_t hook) {
	pic_8259_mask (irq);

	irq_add_hook (irq_get_post_hook_list (irq), hook);

	// This will unmask the interupt now that there is a post hook to run.
	pic_8259_unmask (irq);
}

__attribute__((optimize("O3")))
void irq_handler (irq_t irq) {
	volatile linked_list_irq_hook_t *pre_hook_list = irq_get_pre_hook_list (irq);
	volatile linked_list_irq_hook_t *post_hook_list = irq_get_post_hook_list (irq);

	// If there is no start on pre or post hooks, then the lists is empty.

	if (pre_hook_list->start)
		irq_run_hooks (irq, pre_hook_list);

	enable_int ();
	pic_8259_send_eoi (irq);

	if (post_hook_list->start)
		irq_run_hooks (irq, post_hook_list);
}

// vim: set ts=4 sw=4 noet syn=c:
