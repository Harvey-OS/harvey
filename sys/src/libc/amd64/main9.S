#define NPRIVATES 16

.text

// %rdi and %rsi are set up as argc, argv for main.
.globl	_main
_main:
	movq	%rdx, _tos
	movq 	%rdx, _privates
	movl 	$NPRIVATES, _nprivates
	call	main

loop:
	movq	$_exits, %rdi
	call	exits
	jmp	loop

.data
_exits:
	.ascii	"main"


