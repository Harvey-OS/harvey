 == */ ==
MUL LONG (3)
   NAME "l" 0 <11> LONG
   NAME "r" 4 <11> LONG

 == */ ==
DIV LONG (3)
   NAME "x" 8 <11> LONG
   NAME "y" 12 <11> LONG

 == */ ==
MUL LONG (6)
   MUL LONG (3)
      NAME "l" 0 <11> LONG
      NAME "r" 4 <11> LONG
   DIV LONG (3)
      NAME "x" 8 <11> LONG
      NAME "y" 12 <11> LONG

 == */ ==
DIV LONG (3)
   NAME "z" 16 <11> LONG
   INDEX <9> LONG
      ADDR <13> IND LONG
         NAME "a" 0 <10> LONG
         Z
      NAME "y" 12 <11> IND LONG

 == */ ==
MUL LONG (6)
   MUL LONG (6)
      MUL LONG (3)
         NAME "l" 0 <11> LONG
         NAME "r" 4 <11> LONG
      DIV LONG (3)
         NAME "x" 8 <11> LONG
         NAME "y" 12 <11> LONG
   DIV LONG (3)
      NAME "z" 16 <11> LONG
      INDEX <9> LONG
         ADDR <13> IND LONG
            NAME "a" 0 <10> LONG
            Z
         NAME "y" 12 <11> IND LONG

 == */ ==
MUL LONG (6)
   MUL LONG (6)
      MUL LONG (6)
         MUL LONG (3)
            NAME "l" 0 <11> LONG
            NAME "r" 4 <11> LONG
         DIV LONG (3)
            NAME "x" 8 <11> LONG
            NAME "y" 12 <11> LONG
      DIV LONG (3)
         NAME "z" 16 <11> LONG
         INDEX <9> LONG
            ADDR <13> IND LONG
               NAME "a" 0 <10> LONG
               Z
            NAME "y" 12 <11> IND LONG
   INDEX <9> LONG
      ADDR <13> IND LONG
         NAME "a" 0 <10> LONG
         Z
      NAME "l" 0 <11> IND LONG

	TEXT	f+0(SB),0,$16
	MOVL	l+0(FP),BP
	MOVL	y+12(FP),BX
	MOVL	BP,AX
	IMULL	r+4(FP),
	MOVL	AX,.safe+-8(SP)
	MOVL	DX,.safe+-12(SP)
	MOVL	x+8(FP),AX
	CDQ	,
	IDIVL	BX,
	MOVL	AX,.safe+-4(SP)
	MOVL	.safe+-12(SP),DX
	MOVL	.safe+-8(SP),AX
	IMULL	.safe+-4(SP),
	MOVL	AX,.safe+-8(SP)
	MOVL	DX,.safe+-12(SP)
	MOVL	z+16(FP),AX
	CDQ	,
	IDIVL	a+0(SB)(BX*4),
	MOVL	AX,.safe+-4(SP)
	MOVL	.safe+-12(SP),DX
	MOVL	.safe+-8(SP),AX
	IMULL	.safe+-4(SP),
	IMULL	a+0(SB)(BP*4),
	RET	,
	RET	,
	END	,
