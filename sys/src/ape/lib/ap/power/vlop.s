#define	BDNZ	BC	16,0,

/*
 * 64/64 division adapted from powerpc compiler writer's handbook
 *
 * (R3:R4) = (R3:R4) / (R5:R6) (64b) = (64b / 64b)
 * quo dvd dvs
 *
 * Remainder is left in R7:R8
 *
 * Code comment notation:
 * msw = most-significant (high-order) word, i.e. bits 0..31
 * lsw = least-significant (low-order) word, i.e. bits 32..63
 * LZ = Leading Zeroes
 * SD = Significant Digits
 *
 * R3:R4 = dvd (input dividend); quo (output quotient)
 * R5:R6 = dvs (input divisor)
 *
 * R7:R8 = tmp; rem (output remainder)
 */

TEXT	_divu64(SB), $0
	MOVW	a+0(FP), R3
	MOVW	a+4(FP), R4
	MOVW	b+8(FP), R5
	MOVW	b+12(FP), R6

	/*  count the number of leading 0s in the dividend */
	CMP	R3, $0 	/*  dvd.msw == 0? 	R3, */
	CNTLZW 	R3, R11 	/*  R11 = dvd.msw.LZ */
	CNTLZW 	R4, R9 	/*  R9 = dvd.lsw.LZ */
	BNE 	lab1 	/*  if(dvd.msw != 0) dvd.LZ = dvd.msw.LZ */
	ADD 	$32, R9, R11 	/*  dvd.LZ = dvd.lsw.LZ + 32 */

lab1:
	/*  count the number of leading 0s in the divisor */
	CMP 	R5, $0 	/*  dvd.msw == 0? */
	CNTLZW 	R5, R9 	/*  R9 = dvs.msw.LZ */
	CNTLZW 	R6, R10 	/*  R10 = dvs.lsw.LZ */
	BNE 	lab2 	/*  if(dvs.msw != 0) dvs.LZ = dvs.msw.LZ */
	ADD 	$32, R10, R9 	/*  dvs.LZ = dvs.lsw.LZ + 32 */

lab2:
	/*  determine shift amounts to minimize the number of iterations */
	CMP 	R11, R9 	/*  compare dvd.LZ to dvs.LZ */
	SUBC	R11, $64, R10	/*  R10 = dvd.SD */
	BGT 	lab9 	/*  if(dvs > dvd) quotient = 0 */
	ADD 	$1, R9 	/*  ++dvs.LZ (or --dvs.SD) */
	SUBC 	R9, $64, R9 	/*  R9 = dvs.SD */
	ADD 	R9, R11 	/*  (dvd.LZ + dvs.SD) = left shift of dvd for */
			/*  initial dvd */
	SUB		R9, R10, R9 	/*  (dvd.SD - dvs.SD) = right shift of dvd for */
			/*  initial tmp */
	MOVW 	R9, CTR 	/*  number of iterations = dvd.SD - dvs.SD */

	/*  R7:R8 = R3:R4 >> R9 */
	CMP 	 R9, $32
	ADD	$-32, R9, R7
	BLT	lab3 	/*  if(R9 < 32) jump to lab3 */
	SRW	R7, R3, R8 	/*  tmp.lsw = dvd.msw >> (R9 - 32) */
	MOVW 	$0, R7 	/*  tmp.msw = 0 */
	BR 	lab4
lab3:
	SRW	R9, R4, R8 	/*  R8 = dvd.lsw >> R9 */
	SUBC	R9, $32, R7
	SLW	R7, R3, R7		/*  R7 = dvd.msw << 32 - R9 */
	OR	R7, R8 		/*  tmp.lsw = R8 | R7 */
	SRW	R9, R3, R7		/*  tmp.msw = dvd.msw >> R9 */

lab4:
	/*  R3:R4 = R3:R4 << R11 */
	CMP	R11,$32
	ADDC	$-32, R11, R9
	BLT 	lab5 	/*  (R11 < 32)? */
	SLW	R9, R4, R3	/*  dvd.msw = dvs.lsw << R9 */
	MOVW 	$0, R4 	/*  dvd.lsw = 0 */
	BR 	lab6

lab5:
	SLW	R11, R3	/*  R3 = dvd.msw << R11 */
	SUBC	R11, $32, R9
	SRW	R9, R4, R9	/*  R9 = dvd.lsw >> 32 - R11 */
	OR	R9, R3	/*  dvd.msw = R3 | R9 */
	SLW	R11, R4	/*  dvd.lsw = dvd.lsw << R11 */

lab6:
	/*  restoring division shift and subtract loop */
	MOVW	$-1, R10
	ADDC	$0, R7	/*  clear carry bit before loop starts */
lab7:
	/*  tmp:dvd is considered one large register */
	/*  each portion is shifted left 1 bit by adding it to itself */
	/*  adde sums the carry from the previous and creates a new carry */
	ADDE 	R4,R4 	/*  shift dvd.lsw left 1 bit */
	ADDE 	R3,R3 	/*  shift dvd.msw to left 1 bit */
	ADDE 	R8,R8 	/*  shift tmp.lsw to left 1 bit */
	ADDE 	R7,R7 	/*  shift tmp.msw to left 1 bit */
	SUBC	R6, R8, R11	/*  tmp.lsw - dvs.lsw */
	SUBECC	R5, R7, R9	/*  tmp.msw - dvs.msw */
	BLT 	lab8 	/*  if(result < 0) clear carry bit */
	MOVW	R11, R8 	/*  move lsw */
	MOVW	R9, R7	/*  move msw */
	ADDC 	$1, R10, R11 	/*  set carry bit */
lab8:
	BDNZ 	lab7

	ADDE 	R4,R4 	/*  quo.lsw (lsb = CA) */
	ADDE 	R3,R3 	/*  quo.msw (lsb from lsw) */

lab10:
	MOVW	qp+16(FP), R9
	MOVW	rp+20(FP), R10
	CMP	R9, $0
	BEQ	lab11
	MOVW	R3, 0(R9)
	MOVW	R4, 4(R9)
lab11:
	CMP	R10, $0
	BEQ	lab12
	MOVW	R7, 0(R10)
	MOVW	R8, 4(R10)
lab12:
	RETURN

lab9:
	/*  Quotient is 0 (dvs > dvd) */
	MOVW	R4, R8	/*  rmd.lsw = dvd.lsw */
	MOVW	R3, R7	/*  rmd.msw = dvd.msw */
	MOVW	$0, R4	/*  dvd.lsw = 0 */
	MOVW	$0, R3	/*  dvd.msw = 0 */
	BR	lab10
