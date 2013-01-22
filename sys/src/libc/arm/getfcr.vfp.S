/* for VFP */
#define VMRS(fp, cpu) WORD $(0xeef00a10 | (fp)<<16 | (cpu)<<12) /* FP → arm */
#define VMSR(cpu, fp) WORD $(0xeee00a10 | (fp)<<16 | (cpu)<<12) /* arm → FP */

#define Fpscr 1

TEXT	setfcr(SB), $0
	VMSR(0, Fpscr)
	RET

TEXT	getfcr(SB), $0
	VMRS(Fpscr, 0)
	RET

TEXT	getfsr(SB), $0
	VMSR(0, Fpscr)
	RET

TEXT	setfsr(SB), $0
	VMRS(Fpscr, 0)
	RET
