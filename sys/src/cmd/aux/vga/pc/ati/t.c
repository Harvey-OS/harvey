#include <ctype.h>
#include <stdio.h>
#include <conio.h>
#include <dos.h>

main() {
	struct WORDREGS r;

	r.ax = 0x53;
	int86(0x10, (union REGS *)&r, (union REGS *)&r);
}
