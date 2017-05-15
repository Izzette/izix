// kerenl/arch/x86/asm/kthread_bootstrap.s

.file		"kthread_bootstrap.s"

.code32

	.globl	kthread_bootstrap
	.type	kthread_bootstrap, @function
// void kthread_bootstrap (void (*) ()) {
kthread_bootstrap:
	call	*0x4(%esp)
	
	call	kthread_end_task
	// kthread_end_task doesn't return ...
// }
	.size	kthread_bootstrap, .-kthread_bootstrap

// vim: set ts=8 sw=8 noet syn=asm:
