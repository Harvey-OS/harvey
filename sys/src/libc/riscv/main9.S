.text

.globl _gp
.globl	_main
_main:
	j main
////        auipc   gp,0xfffff
//        addi    gp,gp,_gp
	ret
