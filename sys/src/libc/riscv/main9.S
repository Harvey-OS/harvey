.text

.globl _gp
.globl	_main
.globl _tos
_main:
	lui     a5,%hi(_tos)
	sd      a2,%lo(_tos)(a5)
	ld	a0, 0(sp)
	addi	a1, sp, 8
	addi    sp, sp, -128
	lui     a5,%hi(_privates)
	sd      sp,%lo(_privates)(a5)

	li      a3, 16
	lui     a5,%hi(_nprivates)
	sd      a3,%lo(_nprivates)(a5)
	j main
	addi    sp, sp, 128
////        auipc   gp,0xfffff
//        addi    gp,gp,_gp
	ret
