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
#include "comm.h"
#include "path.h"
#include "graphics.h"
#include "cache.h"

#ifdef VAX
int ishift[] = {0, 8, 16, 24};
extern unsigned char bytemap[];
#else
int ishift[] = {24, 16, 8, 0};
#endif

struct object
imageproc(void)
{
	struct object func;
	func = pop();
	if(func.type == OB_ARRAY && func.xattr == XA_EXECUTABLE)return(func);
	if(func.type == OB_STRING && func.xattr != XA_EXECUTABLE)return(func);
	fprintf(stderr,"bad imagemask proc type %d attr %d\n",func.type,
		func.xattr);
	pserror("typecheck", "imageproc");
}

void
imagemaskOP(void)
{
	enum bool invert;
	struct object matrix, object;
	struct object func, result;
	int i, k, j, count, bottomup, lefttoright, rotate=0, rheight, rwidth;
	int bcount, *shift, mask, width, height, gindex, upsidedown=0;
	int kmax, data, edge, val, last, rdata;
	double psy, f1, yr, btop, xadj, orx, ory;
	double xbits,ybits,dithmax, xs, ys, x1, y1, xo, yo, x, y,sx,dwidth,dheight;
	double xedge;
	unsigned char *cp;
	Bitmap *bm;
	int fun, fun1;

	func = imageproc();
	matrix = realmatrix(pop());
	object = pop();
	if((invert = object.value.v_boolean) == FALSE)fun = fun1 = S|D;
	else fun = fun1 = S|D;
	gindex = floor(currentgray()*((double)GRAYLEVELS-1.)/51.2);
	if(gindex >= 4)fun= fun1 = DandnotS;
	if(E(matrix, 3)*G[3] > SMALL){
		bottomup = 1;
		if((fabs(G[3])- 1.)>SMALL)y = 1.;	/*fixes tex imagemask - all is unity*/
		else y = 0.;
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
	object = integer("imagemask");
	height = object.value.v_int;
	object = integer("imagemask");
	width = object.value.v_int;
	if(Graphics.device.width == 0||charpathflg)
		return;
	count = bcount = 0;
	if(fabs(G[0]) > SMALL)xbits = fabs(G[0]/E(matrix,0)) + SMALLDIF;
	else xbits = fabs(G[1]/E(matrix,0)) + SMALLDIF;
	if(fabs(G[3]) > SMALL)ybits = fabs(G[3]/E(matrix,3)) + SMALLDIF;
	else ybits = fabs(G[2]/E(matrix,3)) + SMALLDIF;
	if(xbits == 0.)xbits++;
	if(ybits == 0.)ybits++;
	edge = width;
	dwidth = (double)width;
	dheight = (double)height;
	rheight = (int)ceil(dheight*ybits);		/*added ceil*/
	rwidth = (int)ceil(dwidth*xbits);
if(mdebug){
printCTM("ctm");
printmatrix("imagmatrix", matrix);
}
	if(edge%8 != 0)
		edge += 8 - edge%8;
	if(G[0] < 0. && G[3] <0. && fabs(G[0])>SMALL && fabs(G[3])>SMALL){
		upsidedown=1;
		if(E(matrix,3)*G[3] > 0.){
			bottomup=1;
			y = 1.;
		}
		else{ bottomup=0; y=0;}
		lefttoright=0;
	}
	if(G[0] <= SMALL && G[3] <= SMALL && !upsidedown){
		x=1.; y=1.;
		if(G[1] < 0.)
		ys = (double)Graphics.device.height-(G[1]*x+G[3]*y+G[5])
		    +(double)anchor.y;
		else ys = (double)Graphics.device.height-(G[1]*x+G[3]*y+G[5]+
		    dwidth*ybits) + (double)anchor.y;

		if(bottomup)ys += E(matrix,5)*ybits;
		xs = sx = G[0]*x + G[2]*y + G[4];
		nxadj = G[2]*y;
	}
	else {
		if(!bottomup)ys = (double)Graphics.device.height-(G[1]*x+G[3]*y+G[5]+
			E(matrix,5)*ybits) +(double)anchor.y;
		else ys = (double)Graphics.device.height-(G[1]*x+G[3]*y+G[5])+(double)anchor.y;
		nxadj = E(matrix,4)*xbits;
		xs = sx = G[0]*x + G[2]*y + G[4] + nxadj;
	}
	btop = E(matrix,5)*ybits;
	xadj = yadj = 0.;
	orx = xs;
	if(cacheit > 0)
		ory = (double)Graphics.device.height -
		    (G[5] + currentcache->origin.y);
	else ory = ys;
	if(G[0] <= SMALL && G[3] <= SMALL && !upsidedown){
		rotate = 1;
		btop = 0.;
		xadj = E(matrix,5)*xbits;
		if(cacheit > 0)
			orx = xs - currentcache->origin.x;
		if(G[1] < 0.){
			bottomup = 1;
			if(cacheit<0){
				orx = xs - forigin.x + (xadj-rheight);
			}
			else if(!cacheit){
				orx -= rheight;
				ory -= rwidth;
				xs -= rheight;
				ys -= rwidth;
			}
			else {
				yadj = E(matrix,4)*xbits;
				ory += yadj;
				yadj = -yadj;
			}
			xs += xadj;
		}
		if(G[2] < 0.){
			lefttoright = 0;
			yadj = dwidth*ybits;
			if(cacheit>0){
				ory -= (yadj+E(matrix,4)*xbits);
			}
			else if(cacheit<0){
				orx -= (xadj - (fw-forigin.x));
				ory -= (rwidth-yadj);
			}
			else if(!cacheit){
				ory += yadj;
				ys += 2*yadj;
			}
		}
	}
	if(cacheit>0){
		if(texcode != 040 && ((!rotate && rheight > currentcache->cheight) ||
			(rotate && rheight > currentcache->cwidth)|| rwidth > currentcache->cwidth) ){
			if(mdebug)
			fprintf(stderr,"too high for cache %o rot %d ch %d rh %d cw %d rw %dxs %f mat %f\n",
			   texcode,rotate,currentcache->cheight,rheight,currentcache->cwidth,rwidth,xs, E(matrix,4)*xbits);
			bm = &screen;
			if(texfont)xs = xs + anchor.x - 2*E(matrix,4)*xbits;
			else xs += E(matrix,4)*xbits + anchor.x;
			if(cacheit != 2)ys += ceil(fh - forigin.y);
			else if(bottomup)ys += btop;
			cacheit=0;
		}
		else {
		fun1=S;
		xs = (double)currentcache->charno*currentcache->cwidth;
		currentcache->cachec[currentcache->charno].edges.min.x =
			(int)floor(xs);
		currentcache->cachec[currentcache->charno].edges.min.y = 0;
		if(rotate){
			if(bottomup)xs += xadj;
			else xs += currentcache->cwidth - xadj;
			if(!lefttoright){
				ys = yadj;
				yadj += E(matrix,4)*xbits;
			}
			else ys = 0.;
		}
		else{
			if(bottomup)ys = dheight*ybits;
			else ys = currentcache->cheight - btop;
			if(upsidedown){
				orx -= currentcache->cwidth+2*E(matrix,4)*xbits;
				btop =currentcache->cheight - fabs(rheight - btop);
				ys = currentcache->cheight - fabs(rheight - btop);
				nxadj = -nxadj;
			}
			if(!lefttoright)xs += currentcache->cwidth-1;
			if(cacheit == 2){		/*tex adjustment*/
				if(bottomup){
					ys =currentcache->cheight-2;
					ory += btop;
					orx -= 2*E(matrix,4)*xbits;
					nxadj = -nxadj;
					currentcache->cachec[currentcache->charno].texy = btop;
				}
				else {ys = 0;
					nxadj = -nxadj;
					ory = ory - btop;
					orx -= 2*E(matrix,4)*xbits;
			 		currentcache->cachec[currentcache->charno].texy =currentcache->cheight - btop;
				}
			}
		}
		bm = currentcache->charbits;
		}
	}	
	else{
		bm = &screen;
		if(!cacheit){
			xs += fabs(E(matrix,4));
			if(bottomup && !texfont)ys += (dheight-1)*ybits;
		}
		if(cacheit < 0 && !texfont){
			if(rotate){
				if(bottomup)
					xs -= forigin.x;
				else xs -= floor(xadj - (fh-forigin.x));
				if(!lefttoright)ys += yadj+btop-E(matrix,4)*xbits;
				else ys += E(matrix,4)*xbits;
			}
			else{
				if(!upsidedown){
					ys += ceil(fh - forigin.y);
				}
				else{ ys += ceil(btop - forigin.y);
					xs -= 2*E(matrix,4)*xbits;
				}
			}
		}
		if(cacheit<0 && texfont){
			if(bottomup)ys += btop;	/*makes tex work - wrong!*/
			xs -= 2*E(matrix,4)*xbits;
		}
		yr = ys - anchor.y;
		if(xs < 0.)xs=0.;
		if(!bottomup && !rotate){
			if(yr < Graphics.iminy || yr+rheight > Graphics.imaxy)return;
		}
		else if(yr > Graphics.imaxy || yr-rheight < Graphics.iminy)return;
		if(lefttoright||(rotate && !bottomup)){
			if(xs < Graphics.iminx /*|| xs+rwidth-2 > Graphics.imaxx*/)return;
		}
		else if(xs > Graphics.imaxx || xs-rwidth < Graphics.iminx)return;
		xs += anchor.x;
	}
if(bbflg){
fprintf(stderr,"(%f %f) (%f %f)\n",orx,(double)Graphics.device.height-(ory+rheight),orx+rwidth, (double)Graphics.device.height-ory);
	return;
}
	if(lefttoright)xedge = xs + dwidth*xbits;
	else xedge = xs - dwidth*xbits;
	xo = xs; yo = ys;
	f1 = -1.;
	x1 = xs + xbits;
	yr = ys + ybits;
if(mdebug){fprintf(stderr,"lefttoright %d bottomup %d rotate %d upsidedown %d cacheit %d texfont %d xb %f yb %f\n",lefttoright,bottomup, rotate, upsidedown,cacheit,texfont,xbits, ybits);
fprintf(stderr,"ys %f btop %f ory %f\n",ys, btop,ory);
}
				/*make sure 1st line used:makes chars look better*/
	if(rotate){
		if((int)x1 == (int)xs)x1 = xs + 1.;
	}
	else if((int)floor(yr) == (int)floor(ys))yr = ys + 1.;
	while(count < edge * height){
		execpush(func);
		execute();
		result = pop();
		if(result.type !=  OB_STRING){
			fprintf(stderr,"imagemask proc returned type %d\n",result.type);
			pserror("typecheck", "imagemask");
		}
		if(result.value.v_string.length == 0)return;
		cp = result.value.v_string.chars;
		last = -1;
		for(j=0;j<result.value.v_string.length;j++, cp++){
		    mask = 0200;
		    for(k=0; k<8; k++){
			val = *cp&mask;
			if(invert==FALSE)val = val==0?1:0;
			if(val){
				if(f1 < 0.){
					if(rotate)f1 = ys;
					else f1 = xs;
				}
			}
			if(val==0 && f1 >= 0.){
				if(rotate){
					if((int)floor(x1)!=(int)floor(xs) && (int)floor(f1)!=(int)floor(ys)){
					 if(lefttoright)
						rectf(bm,Rect((int)floor(xs),(int)floor(f1),
						  (int)floor(x1),(int)floor(ys)),fun1);
					else
					   rectf(bm,Rect((int)floor(xs),(int)floor(ys),(int)floor(x1),
						(int)floor(f1)),fun1);
					}
				}
				else{
				if((int)floor(f1) != (int)floor(xs) && (int)floor(ys) != (int)floor(yr)){
				if(lefttoright){
				  rectf(bm, Rect((int)floor(f1),(int)floor(ys),(int)floor(xs),(int)floor(yr))
					,fun1);
				}
				else{ rectf(bm, Rect((int)floor(xs),(int)floor(ys),(int)floor(f1),
					(int)floor(yr)), fun1);
				}
				}
				}
				f1 = -1.;
			}
			if(rotate){
				if(lefttoright)ys += ybits;
				else ys -= ybits;
				if(ys < 0.)ys = 0.;
				yr = ys+ybits;
			}
			else {
				if(lefttoright){
					if(xs < xedge)xs += xbits;
				}
				else if(xs > xedge)xs -= xbits;
				if(xs < 0.)xs = 0.;
			}
			mask >>= 1;
			count++;
			if(++bcount >= edge){
				bcount = 0;
				if(rotate){
					if(f1 >= 0. && (int)floor(x1)!=(int)floor(xs) &&
					  (int)floor(f1)!=(int)floor(ys)){
					 if(lefttoright)
						rectf(bm,Rect((int)floor(xs),(int)floor(f1),
						  (int)floor(x1),(int)floor(ys)),fun1);
					else
					   rectf(bm,Rect((int)floor(xs),(int)floor(ys),(int)floor(x1),
						(int)floor(f1)),fun1);
					}
				}
				else{
				if(f1 >= 0. && (int)floor(f1) != (int)floor(xs) &&
				  (int)floor(ys) != (int)floor(yr)){
				if(lefttoright){
				  rectf(bm, Rect((int)floor(f1),(int)floor(ys),(int)floor(xs),(int)floor(yr))
					,fun1);
				}
				else{ rectf(bm, Rect((int)floor(xs),(int)floor(ys),(int)floor(f1),
					(int)floor(yr)), fun1);
				}
				}
				}
				last = -1;
				if(rotate){
					if(!bottomup)xs += xbits;
					else xs -= xbits;
					if(xs < 0.)xs = 0.;
					x1 = xs + xbits;
					ys = yo;
				}
				else {
					if(!bottomup)ys += ybits;
					else ys -= ybits;
					if(ys < 0.)ys = 0.;
					xs = xo;
					yr = ys+ybits;
				}
				f1 = -1.;
				if(count >= height*edge)goto done;
			}
		    }
		}
	}
done:
	if(rotate){
		if(lefttoright)yo += dwidth*ybits;
		cborder.max.y = (int)floor(yo);
		cborder.max.x=(int)floor(dheight*xbits);
	}
	else {
		if(!lefttoright)xo -= dwidth*xbits;
		else xo += dwidth*xbits;
		if(ys>(double)Graphics.device.height)
			ys = (double)Graphics.device.height;
		cborder.max.x = (int)floor(xo);
		cborder.max.y = (int)floor(ys+1);
	}
	if(cacheit>0){
		currentcache->cachec[currentcache->charno].edges.max.x =
			cborder.max.x;
		currentcache->cachec[currentcache->charno].edges.max.y = 
			currentcache->cheight+1;
	}
	else {
		cborder.min.x = (int)floor(orx+.5);
		cborder.min.y = (int)floor(ory+.5);
	}
	if(rotate){
		i = rwidth;
		rwidth = rheight;
		rheight = i;
	}
	if(cacheit>0){
		cborder.min.x = (int)floor(orx+.5);
		cborder.min.y = (int)floor(ory+.5);
		currentcache->cachec[currentcache->charno].edges.max.x=
			currentcache->cachec[currentcache->charno].edges.min.x+rwidth;
		currentcache->cachec[currentcache->charno].edges.max.x=
			currentcache->cachec[currentcache->charno].edges.min.x+
			currentcache->cwidth;
		if(Graphics.clipchanged){
			psy = Graphics.device.height + anchor.y - cborder.min.y;
			clipchar(sx, psy, dheight*ybits, dwidth*xbits);
			if(Graphics.path.first == NULL){
				return;
			}
		}
		cborder.max.x = cborder.min.x+currentcache->cwidth+4;
		cborder.max.y += cborder.min.y;
		cborder.min.y += anchor.y;
		if(cborder.min.x >= Graphics.imaxx||
			cborder.min.y < Graphics.iminy ||
			cborder.min.y > Graphics.imaxy ||
			cborder.min.x < Graphics.iminx){
				return;
		}
		cborder.min.x += anchor.x;
		if(cacheit == 2 && bottomup){
		     bitblt(&screen, Pt(cborder.min.x,cborder.min.y
			-currentcache->cheight),
			bm, currentcache->cachec[currentcache->charno].edges, fun);
			cborder.min.y -= currentcache->cheight+1;
		}
		else bitblt(&screen, cborder.min, bm,
			currentcache->cachec[currentcache->charno].edges, fun);
	}
#ifndef Plan9
	ckbdry(cborder.min.x,cborder.min.y);
	ckbdry(cborder.max.x,cborder.max.y);
#endif
}
void
printCTM(char *label)
{
	int i;
	fprintf(stderr,"\n%s CTM= ",label);
	for(i=0;i<=5;i++){
		fprintf(stderr," %f ",G[i]);
	}
	fprintf(stderr,"\n");
	fflush(stderr);
}
