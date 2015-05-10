#define NPRIVATES	16

.text

.globl	_main
_main:
	subl	$(8+NPRIVATES*4), %esp
	movl	%eax, _tos
	leal	8(%esp), %eax
	movl	%eax, _privates
	movl	$NPRIVATES, _nprivates
	movl	12(%esp), %eax
	movl	%eax, 0(%esp)
	leal	16(%esp), %eax
	movl	%eax, 4(%esp)
	call	main

loop:
	movl	$_exits, %eax
	movl	%eax, 0(%esp)
	call	exits
	jmp	loop

.data
_exits:
	.ascii	"main"

	/* In Plan9 this is an array */
	/* (<> to fill with an unique integer) */
	/* DATA	_exits<>+0(SB)/4, $"main" */
	/* GLOBL	_exits<>+0(SB), $5 */
	
	/* Displaying it deployed */
	/* DATA	_exits<>+0(SB)/1, $"m" */
	/* DATA	_exits<>+0(SB)/1, $"a" */
	/* DATA	_exits<>+0(SB)/1, $"i" */
	/* DATA	_exits<>+0(SB)/1, $"n" */
	/* GLOBL	_exits<>+0(SB), $5 */

