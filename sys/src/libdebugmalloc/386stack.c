#include <u.h>
#include <libc.h>

int
_malloc_getcallers(ulong *pc, int mpc)
{
	ulong l, v;
	ulong top;
	ulong caller;
	uchar *p;
	int npc;
	ulong x;

	l = (ulong)&l;
	if((l>>24) == 0x7F) /* in process stack */
		top = 0x7ffff000;
	else
		top = (ulong)sbrk(0);

	for(npc=0, l=(ulong)&l; npc < mpc && l<top; l+=4) {
		v = *(ulong*)l;
		if(0x1005 <= v && v < (ulong)end){
			/*
			 * Pick off general CALL (0xE8) and CALL indirect
			 * through AX (0xFFD0).
			 *
			 * The call address must be before the lastpc we found (so that a call to
			 * it could reach lastpc) or must point at a SUBL instruction
			 * that deals with the stack: 0x8[31] 0xEC.
			 * For CALL* AX, we have little choice but to accept it.
			 */
			p = (uchar*)v;
			caller = 0;
			if(*(p-5) == 0xE8) {
				x = (ulong)p + *(ulong*)(p-4);
				if(0x1000 <= x && x < (ulong)end 
				&& (*(uchar*)x & 0xFD) == 0x81 && *(uchar*)(x+1) == 0xEC)
					caller = (ulong) p-5;
			}
			if(caller == 0 && *(p-2) == 0xFF && *(p-1) == 0xD0)
				caller = (ulong) p-2;
			if(caller)
				pc[npc++] = caller;
		}
	}
	return npc;
}
