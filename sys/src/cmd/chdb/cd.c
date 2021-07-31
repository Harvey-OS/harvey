#include	<u.h>
#include	<libc.h>
#include	"gen.h"

schar	dir[] =
{
	U1|L1, U1|L2, U1   , U1   , U1   , U1   , U1|R2, U1|R1,
	U2|L1, U2|L2, U2   , U2   , U2   , U2   , U2|R2, U2|R1,
	   L1,    L2,     0,     0,     0,     0,    R2,    R1,
	   L1,    L2,     0,     0,     0,     0,    R2,    R1,
	   L1,    L2,     0,     0,     0,     0,    R2,    R1,
	   L1,    L2,     0,     0,     0,     0,    R2,    R1,
	D2|L1, D2|L2, D2   , D2   , D2   , D2   , D2|R2, D2|R1,
	D1|L1, D1|L2, D1   , D1   , D1   , D1   , D1|R2, D1|R1,
};
schar	attab[] =
{
	U2R1, -15,
	U1R2, -6,
	D1R2, 10,
	D2R1, 17,
	D2L1, 15,
	D1L2, 6,
	U1L2, -10,
	U2L1, -17,

	ULEFT, -9,
	URIGHT, -7,
	DLEFT, 7,
	DRIGHT, 9,

	UP, -8,
	LEFT, -1,
	RIGHT, 1,
	DOWN, 8,
};
Posn	initp =
{
	INIT+BROOK,
	INIT+BKNIGHT,
	INIT+BBISHOP,
	INIT+BQUEEN,
	INIT+BKING,
	INIT+BBISHOP,
	INIT+BKNIGHT,
	INIT+BROOK,
	INIT+BPAWN,
	INIT+BPAWN,
	INIT+BPAWN,
	INIT+BPAWN,
	INIT+BPAWN,
	INIT+BPAWN,
	INIT+BPAWN,
	INIT+BPAWN,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	INIT+WPAWN,
	INIT+WPAWN,
	INIT+WPAWN,
	INIT+WPAWN,
	INIT+WPAWN,
	INIT+WPAWN,
	INIT+WPAWN,
	INIT+WPAWN,
	INIT+WROOK,
	INIT+WKNIGHT,
	INIT+WBISHOP,
	INIT+WQUEEN,
	INIT+WKING,
	INIT+WBISHOP,
	INIT+WKNIGHT,
	INIT+WROOK,
	0,	/* eppos */
	0,	/* moveno */
	0,	/* moves */
};

schar	color[] =
{
	 0,
	 1,  1,  1,  1,  1,  1,  1,
	 0,
	-1, -1, -1, -1, -1, -1, -1,
	 0,
	 1,  1,  1,  1,  1,  1,  1,
	 0,
	-1, -1, -1, -1, -1, -1, -1,
};
