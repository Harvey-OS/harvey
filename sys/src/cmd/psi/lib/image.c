#include <u.h>
#include <libc.h>
#include "system.h"
#ifdef Plan9
#include <libg.h>
#endif
#include "stdio.h"
#include "defines.h"
#include "object.h"
#include "njerq.h"
#include "path.h"
#include "comm.h"
#include "graphics.h"

#define	DITHSIZE8	16
int dithmax8 =	DITHSIZE8*DITHSIZE8-1;
int shift8[] = {0};

#define	DITHSIZE4	4
int dithmax4 =	DITHSIZE4*DITHSIZE4-1;
int shift4[] = {4, 0};

int dithmax2 = 3;
int shift2[] = {6, 4, 2, 0};
int shift1[] = {7, 6, 5, 4, 3, 2, 1, 0};
int dithmax1 = 1;

extern int ishift[];
#ifdef VAX
extern unsigned char bytemap[];
#endif
void
imageOP(void)
{
#ifdef Plan9
	Bitmap *pg;
#else
	Texture *pg;
#endif
	struct object width, height, bits, matrix;
	struct object func, result;
	int i, k, j, l, n, count, bottomup, lefttoright;
	int bcount, *shift, mask, yl, yr;
	int kmax, data, edge, val, last, rdata, toread;
	double xbits, ybits, dithmax, xs, ys, x1, xo, x, y;
	unsigned char *cp;
	Bitmap *bm;

	func = imageproc();
	matrix = realmatrix(pop());
	bits = pop();
	if(E(matrix, 3)*G[3] > 0){
		bottomup = 1;
		y = 1.;
	}
	else{
		bottomup = 0;
		y = 0.;
	}
	if(E(matrix, 0) > 0){
		lefttoright=1;
		x = 0.;
	}
	else{
		lefttoright=0;
		x = 1.;
	}
	height = pop();
	width = pop();
	edge = width.value.v_int;
	switch(bits.value.v_int){
	case 8:
		dithmax = (double)dithmax8;
		mask = dithmax8;
		kmax = 0;
		shift = shift8;
		break;
	case 4:
		dithmax = (double)dithmax4;
		mask = dithmax4;
		kmax = 1;
		shift = shift4;
		if(width.value.v_int&01)edge++;
		break;
	case 2:
		dithmax = (double)dithmax2;
		mask = dithmax2;
		kmax = 3;
		shift = shift2;
		switch(width.value.v_int&03){
		case 1: edge++;
		case 2: edge++;
		case 3: edge++;
		}
		break;
	case 1:
		dithmax = (double)dithmax1;
		mask = dithmax1;
		kmax = 7;
		shift = shift1;
		switch(width.value.v_int&07){
		case 1: edge++;
		case 2: edge++;
		case 3: edge++;
		case 4: edge++;
		case 5: edge++;
		case 6: edge++;
		case 7: edge++;
		}
		break;
	default:
		fprintf(stderr,"%d image not implemented\n",bits.value.v_int);
		return;
	}
	count = bcount = 0;
	if(G[0] > 0.0001 || G[0] < -.001)xbits = G[0]/E(matrix,0);
	else xbits = G[1]/E(matrix,0);
	if(xbits < 0)xbits = -xbits;
	if(G[3] > 0.0001 || G[3] < -.001)ybits = G[3]/E(matrix,3);
	else ybits = G[2]/E(matrix,3);
	if(ybits < 0.)ybits = -ybits;
	if(xbits == 0.)xbits++;
	if(ybits == 0.)ybits++;
	ys = (double)Graphics.device.height - (G[1]*x + G[3]*y + G[5]+E(matrix,5)*ybits);
	if(bottomup)ys += E(matrix,5)*ybits;
	if(ys < 0.)ys = 0.;				/*fudge when G5 too big*/
	xs = G[0]*x + G[2]*y + G[4] + anchor.x;
#ifndef Plan9
	ckbdry((int)ceil(xs), (int)ceil(ys+ybits));
#endif
	bm = &screen;
	xo = x1 = xs;
	yl = (int)floor(ys) + anchor.y;
	yr = (int)floor(ys+ybits) + anchor.y;
	last = -1;
	toread = edge * height.value.v_int;
	while(count < toread){
		execpush(func);
		execute();
		result = pop();
		cp = result.value.v_string.chars;
		if(result.value.v_string.length == 0)
			break;
		for(j=0;j<result.value.v_string.length && count < toread;j++, cp++){
		    rdata = *cp;
		    for(k=0; k<=kmax; k++){
			if(++count > toread)
				break;
			if(bcount < width.value.v_int){
				data = (rdata >> shift[k]) & mask;
				val =(int)floor((double)(data)*4.999/dithmax);
				if(last != val){
					if(last >= 0){
					if(lefttoright)
						texture(bm, Rect((int)floor(x1),yl,
							(int)floor(xo),yr), pg, S);
					else texture(bm, Rect((int)floor(xo),yl,
						(int)floor(x1),yr), pg, S);
					x1 = xo;
					}
					pg = pgrey[val];
					last = val;
				}
				if(lefttoright)xo += xbits;
				else xo -= xbits;
			}
			if(++bcount >= edge){
				bcount = 0;
				if(lefttoright)
				  texture(bm, Rect((int)floor(x1),yl,
					(int)floor(xo),yr), pg, S);
				else texture(bm, Rect((int)floor(xo),yl,
					(int)floor(x1),yr), pg, S);
				last = -1;
				if(!bottomup)ys += ybits;
				else ys -= ybits;
				if(ys < 0.)ys = 0.;
				xo = x1 = xs;
				yl = (int)floor(ys) + anchor.y;
				yr = (int)floor(ys+ybits) + anchor.y;
			}
		    }
		}
	}
#ifndef Plan9
	if(!lefttoright)xo -= (double)width.value.v_int*xbits;
	else xo += (double)width.value.v_int*xbits;
	if(ys > (double)Graphics.device.height)ys = (double)Graphics.device.height;
	ckbdry((int)ceil(xo), (int)ceil(ys));
#endif
}
