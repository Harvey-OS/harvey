	.file	"getcallerpc-MacOSX-386.s"
    .text
.globl _getcallerpc
_getcallerpc:
	movl	4(%esp), %eax
	movl	-4(%eax), %eax
	ret
