#include "lib9.h"

int
_tas(int *la)
{
	int v;

	_asm {
		mov eax, la
		mov ebx, 1
		xchg	ebx, [eax]
		mov	v, ebx
	}
	return v;
}
