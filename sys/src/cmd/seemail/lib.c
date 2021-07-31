#include <u.h>
#include <libc.h>
#include <libg.h>
#include "dat.h"

/*
 * Rand is huge and not worth it here.  Be small.  Borrowed from the white book.
 */
ulong	randn;
void
srand(long n)
{
	randn = n;
}

int
nrand(int n)
{
	randn = randn*1103515245 + 12345;
	return (randn>>16) % n;
}

void
special(SRC *A, SRC *B)
{
	int x, y, k, K, dk, ek;
	SRC pic, *From, *To;

	if(nrand(2)){
		K = 0;
		ek = MAXX/2;
		dk = 1;
		From = A;
		To = B;
	}else{
		K = MAXX/2;
		ek = 0;
		dk = -1;
		From = B;
		To = A;
		memmove(&pic, From, sizeof(SRC));
	}
	for(k=K; k!=ek; k+=dk){
		for(y=0; y<k; y++)
		for(x=0; x<k; x++){
			From->pix[y][x] = To->pix[MAXY/2-k+y][MAXX/2-k+x];
			From->pix[y][MAXX-k+x] = To->pix[y][MAXX/2+x];
			From->pix[MAXY-k+y][x] = To->pix[MAXY/2+y][x];
			From->pix[MAXY-k+y][MAXX-k+x] = To->pix[MAXY/2+y][MAXX/2+x];
		}
		showimage(From, 0);
		if(ek == 0)
			memmove(From, &pic, sizeof(SRC));
	}
}

void
wipe(SRC *From, SRC *To)
{
	int x, y, k;

	showimage(From, 1);
	if(nrand(2)){
		dive(From, To);
		return;
	}
	switch(nrand(6)){
	case 0:
		for(x = 0; x < MAXX; x++){
			for(y = 0; y < MAXY; y++)
				From->pix[y][x] = To->pix[y][x];
			showimage(From, 0);
		}
		break;
	case 1:
		for(y = 0; y < MAXY; y++){
			memmove(From->pix[y], To->pix[y], MAXX);
			showimage(From, 0);
		}
		break;
	case 2:
		for(x = MAXX-1; x >= 0; x--){
			for(y = 0; y < MAXY; y++)
				From->pix[y][x] = To->pix[y][x];
			showimage(From, 0);
		}
		break;
	case 3:
		for(y = MAXY-1; y >= 0; y--){
			memmove(From->pix[y], To->pix[y], MAXX);
			showimage(From, 0);
		}
		break;
	case 4:
		special(From, To);
		break;
	case 5:
		for(k = 0; k < 2*MAXX; k++){
			int Y, Z;
			Y = Min(MAXY, k);
			for(y = 0; y < Y; y++){
				if(k < MAXX)
					for(Z=k-1,x=0 ; x < k-y; x++,Z--)
						From->pix[y][x] = To->pix[Z][MAXX-1-y];
				else if(y < k-MAXX)
					for(x = 0; x < MAXX; x++)
						From->pix[y][x] = To->pix[y][x];
				else{
					for(x = 0; x < k-MAXX; x++)
						From->pix[y][x] = To->pix[y][x];
					for(Z=MAXY-1 ; x < k-y; x++,Z--)
						From->pix[y][x] = To->pix[Z][MAXX-1-y+(k-MAXX)];
				}
			}
			showimage(From, 0);
		}
		break;
	default:
		abort();
	}
}

void
twirl(SRC *From, SRC *To)
{
	int y, cnt;

	for(cnt=0; cnt<MAXY; cnt++){
		for(y=0; y<MAXY; y++)
			memmove(From->pix[y], To->pix[(y+cnt)%MAXY], MAXX);
		showimage(From, 0);
	}
}

void
dive(SRC *From, SRC *To)
{
	int y, k;
	SRC pic;

	USED(From);
	memmove(&pic, To, sizeof(SRC));
	for(k=0; k<MAXY; k++){
		for(y=0; y<k; y++)
			memmove(To->pix[y], pic.pix[k-y], MAXX);
		for(y=k; y<MAXY; y++)
			memmove(To->pix[y], pic.pix[y-k], MAXX);
		showimage(To, 0);
	}
	for(k=0; k<MAXY; k++){
		for(y=0; y<=k; y++)
			memmove(To->pix[MAXY-1-y], pic.pix[k-y], MAXX);
		for(y=k+1; k+y < MAXY; y++)
			memmove(To->pix[MAXY-1-y], pic.pix[k+y], MAXX);
		for( ; y<MAXY; y++)
			memset(To->pix[MAXY-1-y], 0, MAXX);
		showimage(To, 0);
	}
}
