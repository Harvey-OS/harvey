#define NPRIVATES 16

.text

// %rdi and %rsi are set up as argc, argv for main.
// %rdx points to top of stack
.globl	_main
_main:

	movq	0(%rsp), %rdi
	leaq	8(%rsp), %rsi
	movq	%rdx, _tos
	movq 	$-128, _privates
	addq	%rsp, _privates
	movl 	$NPRIVATES, _nprivates
	leaq	-160(%rsp), %rsp
	call	main

loop:
	movq	$_exits, %rdi
	call	exits
	jmp	loop

.data
_exits:
	.ascii	"main"


.globl gettls0
gettls0:
	movq %fs:0, %rax
	ret
