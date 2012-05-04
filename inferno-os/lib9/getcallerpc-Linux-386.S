	.file	"getcallerpc-Linux-386.S"
	.type	getcallerpc,@function
	.global	getcallerpc
getcallerpc:
	movl	4(%esp), %eax
	movl	-4(%eax), %eax
	ret
