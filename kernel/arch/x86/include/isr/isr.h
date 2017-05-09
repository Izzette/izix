// kernel/arch/x86/include/isr/isr.h

#ifndef IZIX_ISR_H
#define IZIX_ISR_H 1

void isr_df ();
void isr_np ();
void isr_gp ();

void isr_irq0 ();
void isr_irq1 ();
// IRQ2 is used interally by the 8259PIC.
void isr_irq3 ();
void isr_irq4 ();
void isr_irq5 ();
void isr_irq6 ();
void isr_irq7 ();
void isr_irq8 ();
void isr_irq9 ();
void isr_irq10 ();
void isr_irq11 ();
void isr_irq12 ();
void isr_irq13 ();
void isr_irq14 ();
void isr_irq15 ();

#endif

// vim: set ts=4 sw=4 noet syn=c:
