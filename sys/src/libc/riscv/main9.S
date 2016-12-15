.text

.globl _gp
.globl	_main
.globl _tos
_main:
	lui     a5,%hi(_tos)
	sd      a2,%lo(_tos)(a5)
	ld	a0, 0(sp)
	addi	a1, sp, 8
	j main
////        auipc   gp,0xfffff
//        addi    gp,gp,_gp
	ret
