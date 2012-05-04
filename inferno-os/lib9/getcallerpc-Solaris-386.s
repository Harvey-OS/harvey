	.align	4
	.globl	getcallerpc
getcallerpc:
	movl	4(%esp), %eax
	movl	-4(%eax), %eax
	ret	
	.align	4
	.type	getcallerpc,@function
	.size	getcallerpc,.-getcallerpc
