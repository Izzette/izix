// kerenl/arch/x86/asm/kthread_bootstrap.s

.file		"kthread_bootstrap.s"

.code32

	.globl	kthread_bootstrap
	.type	kthread_bootstrap, @function
kthread_bootstrap:
	call	*(%esp)
	add	$0x4,		%esp
	
	call	kthread_end_task
	.size	kthread_bootstrap, .-kthread_bootstrap

// vim: set ts=8 sw=8 noet syn=asm:
