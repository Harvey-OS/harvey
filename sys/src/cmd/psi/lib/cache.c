#include <u.h>
#include <libc.h>
#include "system.h"
#ifdef Plan9
#include <libg.h>
#endif
#include "stdio.h"
#include "defines.h"
#include "njerq.h"
#include "object.h"
#include "comm.h"
#include "path.h"
#include "graphics.h"
#include "cache.h"
#include <ctype.h>

int fw, fh;
struct pspoint forigin;

int
findfno(void)
{
	struct object fid, font;
	struct cachefont *old;
	int i, j;

	if(!msize)return(-1);
	old=currentcache;
	currentfontOP();
	font = pop();
	fid = dictget(font.value.v_dict, FID);
	for(i=0;i<msize;i++){
		if(cachefont[i].cfontid.value.v_fontid.fontno ==
		    fid.value.v_fontid.fontno){
			for(j=0;j<4;j++){
				if(G[j] != cachefont[i].cmatrix[j])break;
			}
			if(j >= 4){
				currentcache = &cachefont[i];
				fontchanged = 0;
				return(i);
			}
		}
	}
	return(-1);
}

void	 	/*called from setcachedev updates msize to save*/
putfcache(int dont, double ury, double urx, double lly, double llx)
{
	struct object font;
	struct object fid, fmatrix, BitMaps, CharDict;
	struct object fontptr, box, matrix, tmatrix, xmatrix, ascentob;
	struct pspoint ur, ll, p1, p2;
	int i, width, height, type, istex=0;
	static int totalbytes, fontct;
	double fwidth, fheight, bx[4], ascent;

	if(msize >= CACHE_MLIMIT){
		if(fontct++ < 5)
			fprintf(stderr,"hit font limit %d increase CACHE_MLIMIT\n",msize);
		dont=1;
	}
	currentfontOP();
	font=pop();
	fid = dictget(font.value.v_dict,FID);
	tmatrix = dictget(font.value.v_dict,FontMatrix);
	box = dictget(font.value.v_dict,FontBBox);
	if(box.type == OB_NONE){
		pserror("no FontBBox", "putfcache");
	}
	for(i=0;i<4;i++)
		if(box.value.v_array.object[i].type == OB_INT)
			bx[i] = (double)I(box,i);
		else bx[i] = E(box,i);
	BitMaps = dictget(font.value.v_dict,cname("BitMaps"));	/*tex*/
	CharDict = dictget(font.value.v_dict, cname("CharDict")); /*Dvilaser*/
	if(BitMaps.type != OB_NONE || CharDict.type != OB_NONE)
		istex = 1;
	tmatrix = dictget(font.value.v_dict,ImageMatrix);
	if(tmatrix.type == OB_NONE && current_type==3 && istex){
		bx[0] = urx;
		bx[1] = ury;
		bx[2] = llx;
		bx[3] = lly;
	}
	p1.x=bx[0];p1.y=bx[1];
	p1 = dtransform(p1);
	p2.x=bx[2]; p2.y=bx[3];
	p2 = dtransform(p2);
	fheight = p1.y - p2.y;
	fwidth = p1.x - p2.x;
	if(fwidth < 0.)fwidth = -fwidth;
	if(fheight < 0.)fheight = -fheight;
	width = (int)ceil(fwidth) + 1;		/*fudge for safety-space too big*/
	height = (int)ceil(fheight);
					/*don't use narrow char for tex size guess*/
					/*was tmatrix.type - BitMaps is better*/
	if(istex && current_type == 3){
		if(!isalnum(texcode) || texcode == '1' || texcode == 'l' || texcode == 'i' || texcode == 'I'){
			dont=1;
		}
		width = (int)ceil(fwidth);
		height = (int)ceil(fheight);
		width *= 2;
		height *= 2;
		if(width <10)width=10;	/*more protection!*/
		if(width < height)width = height;	/*wastes space but safe*/
		if(width > height)height=width;	/*protect against small 1st char*/
	}
	fw = width;
	fh = height;
 if(cacheout)fprintf(fpcache,"height %d width %d ",height,width);
	if(current_type == 3 && (istex||tmatrix.type != OB_NONE)){
		ascentob = dictget(font.value.v_dict,Ascent);
		if(ascentob.type != OB_NONE){
			ascent = ascentob.value.v_real;
			ur.x = 0.;
			ur.y = ascent;
			ur = dtransform(ur);
			forigin.y = ur.y;
			if(G[0] < 0. && G[3] < 0. && fabs(G[0])>SMALL && fabs(G[3])>SMALL){
				forigin.x = width - fabs(ur.x);
				forigin.y = fheight-fabs(ur.y);
			}
			else if(G[0] <= SMALL && G[3] <= SMALL && G[1] < 0.)
				forigin.x = width - fabs(ur.x);
			else forigin.x = fabs(ur.x);
		}
		else forigin.x = forigin.y = 0.;
	}
	if(width*CACHE_CLIMIT > 32768 || dont){
		currentcache = (struct cachefont *)0;
		return;
	}
	cachefont[msize].charbits =cachealloc(Rect(0,0,width*CACHE_CLIMIT,height+3));
	if(cachefont[msize].charbits == 0){
		if(mdebug)fprintf(stderr,"cachealloc failed\n");
		currentcache = (struct cachefont *)0;
		return;
	}
	totalbytes += width*CACHE_CLIMIT*height/8;
	cachefont[msize].origin.x = forigin.x;
	cachefont[msize].origin.y = forigin.y;
	if(istex && current_type == 3)
		cachefont[msize].internal = 0;
	else if(tmatrix.type == OB_NONE && current_type == 3)
		cachefont[msize].internal = 2;
	else cachefont[msize].internal = 1;
	cachefont[msize].cheight = height;
	cachefont[msize].cwidth = width;
	if(current_type == 1||cachefont[msize].internal == 2){
		cachefont[msize].origin.x=fabs(p1.x);
		cachefont[msize].origin.y=fabs(p1.y);
		cachefont[msize].upper.y=p2.y;
		cachefont[msize].upper.x=p2.x;
 if(cacheout)fprintf(fpcache,"orig %d %d %d %d ",(int)p1.x,(int)p1.y,(int)p2.x,(int)p2.y);
	}
	cachefont[msize].cfontid = fid;
	cachefont[msize].cfonts = font;
	for(i=0;i<4;i++)
		cachefont[msize].cmatrix[i] = G[i];
	currentcache = &cachefont[msize];
	cachefont[msize].charno = 0;
	fontchanged = 0;
	msize++;
}
void
mcachechar(void)
{		/*needs to look for low sequence if at limit*/
	struct object font, charno;
	struct cachefont *cache;
	double adj;
	charno = pop();
	cache = currentcache;
	if(Graphics.device.width == 0){
		Graphics.width.x = cache->cachec[cache->charno].gwidth;
		Graphics.width.y = cache->cachec[cache->charno].gheight;
		return;
	}
	adj = yadj;
	cache->cachec[cache->charno].value = charno.value.v_int;
	cache->cachec[cache->charno].lastused = cache->sequence++;
	cache->cachec[cache->charno].yadj = yadj;
	cache->cachec[cache->charno].xadj = nxadj;
	Graphics.width.x = cache->cachec[cache->charno].gwidth;
	Graphics.width.y = cache->cachec[cache->charno].gheight;
	cache->charno++;
}

int
putcachech(void)
{
	register struct chars *cachechar;
	struct object font, charno;
	struct cachefont *cache;
	double psy, psx;
	int i, x, y, delta, gindex;
	unsigned char neg=0;
	chct++;
	charno = pop();
	cache = currentcache;
	if(cache == (struct cachefont *)0||fontchanged||dontcache)return(0);
	for(i=0, cachechar = &cache->cachec[0];i<cache->charno;i++,cachechar++)
		if(cachechar->value == 	charno.value.v_int)break;
	if(i == cache->charno){
		return(0);
	}
	if(Graphics.device.width == 0)
		goto dont;
	if(cache->internal){
		psy = G[5] + cache->origin.y + cache->cachec[i].yadj;
		if(current_type == 3)psx = G[4] - cache->origin.x  +
			cache->cachec[i].xadj;
		else psx = G[4] + cache->origin.x;		/*was +*/
	}
	else { psy = G[5] + cache->cheight - cachechar->texy;
		psx = G[4] - cache->origin.x  +
			cache->cachec[i].xadj;
	}
	if(Graphics.clipchanged){
		clipchar(G[4], psy,
		   (double)(cachechar->edges.max.x-cachechar->edges.min.x),
		   (double)(cachechar->edges.max.y-cachechar->edges.min.y));
		if(Graphics.path.first == NULL){
			goto dont;
		}
	}
	else if(psx < Graphics.iminx|| psx + cache->cwidth > Graphics.imaxx ||
		psy - cache->cheight < Graphics.iminy|| psy > Graphics.imaxy)
		goto dont;
	psy = Graphics.device.height - psy + anchor.y;
	psx += (double)anchor.x;
	if(psy<0)
		goto dont;
	cachechar->lastused = cache->sequence++;
	gindex = floor(currentgray()*((double)GRAYLEVELS-1.)/51.2);
	if(gindex < 0)gindex = 0;
	if(current_type == 1 || cache->internal == 2){
		psy = psy - cache->cheight + 2*cache->origin.y;
		if(current_type == 1)
		psx = psx - 2*cache->origin.x +
			cache->cachec[i].xadj;	/*was- 2**/
		if(cache->upper.y<0){
			psy -= cache->upper.y;
			if(cache->upper.x<0.)
				psy -= cache->origin.y;
		}
		if(cache->upper.x<0){
			psx += cache->upper.x;
			if(cache->upper.y > 0.)
				psx += cache->origin.x;
		}
		y = (int)floor(psy);
	}
	else y = (int)floor(psy+.5);
	x = (int)floor(psx+.5);
/*	if(y<0)goto dont;*/
	if(gindex >= 4)
		bitblt(&screen, Pt(x, y), cache->charbits,cachechar->edges, DandnotS);
	else bitblt(&screen, Pt(x, y), cache->charbits,cachechar->edges, S|D);
#ifndef Plan9
	ckbdry(x, y);
	ckbdry(x+cache->cwidth, y+cache->cheight);
#endif
dont:
	Graphics.width.x = cachechar->gwidth;
	Graphics.width.y = cachechar->gheight;
	return(1);
}

int
getbytes(double urx, double  ury, double llx, double lly)
{
	int cwidth, cheight, cbytes;
			/*useless since whole fonts are cached except to find big fonts*/
	cheight = fabs(ceil(ury-lly));
	cwidth = fabs(ceil(urx-llx));
	if(cwidth<1)cwidth=1;
	if(cheight<1)cheight=1;
	cbytes = ((cwidth + 7) >>3)*cheight;
	return(cbytes);
}
void
BuildChar(void)
{
	struct object charname, fontdir, charno, chdata, array, Bheight;
	struct object height, width, matrix, chdict, encoding, ascentob;
	int gindex;
	struct pspoint ur;
	double ascent;
	charno = pop();
	chct++;
	fontdir = pop();
	encoding = dictget(fontdir.value.v_dict, Encoding);
	if(encoding.type == OB_NONE)
		pserror("show", "BuildChar no encoding");
	push(encoding);
	push(charno);
	getOP();
	charname = pop();
	chdict = dictget(fontdir.value.v_dict, Chardata);
	if(chdict.type == OB_NONE)
		pserror("show", "BuildChar no chardata");
	chdata = dictget(chdict.value.v_dict, charname);
	if(chdata.type == OB_NONE){
		fprintf(stderr,"undefined char \"%s\", using question:line %d:\n",
			charname.value.v_string.chars,Line_no);
		chdata = dictget(chdict.value.v_dict, blank);
		if(chdata.type == OB_NONE)
			pserror("BuildChar","can't find question in font");
	}
	matrix = dictget(fontdir.value.v_dict,ImageMatrix);
	if(matrix.type == OB_NONE)
		pserror("BuildChar", "ImageMatrix wrong");
	matrix.value.v_array.object[4] = chdata.value.v_array.object[3];
	matrix.value.v_array.object[5] = chdata.value.v_array.object[4];
	Bheight = dictget(fontdir.value.v_dict, BHeight);
	if(Bheight.type == OB_NONE)
		pserror("BuildChar", "Bheight wrong");
	width = chdata.value.v_array.object[0];
	height = chdata.value.v_array.object[1];
	push(chdata.value.v_array.object[2]);
	push(makereal(0.0));
					/* char bounding box */
	push(chdata.value.v_array.object[3]);
	push(makereal(0.0));
	push(makereal((double)width.value.v_int));
	push(makereal((double)height.value.v_int));
	setcachedeviceOP();
	push(width);
	push(height);
/*	gindex = floor(currentgray()*((double)GRAYLEVELS-1.)/51.2);
	if(gindex < 0)gindex = 0;
	if(gindex == 4)push(false);
	else push(true);*/
	push(true);
	push(matrix);
	array = makearray(1, XA_EXECUTABLE);
	array.value.v_array.object[0] = chdata.value.v_array.object[5];
	push(array);
	imagemaskOP();
}
void
printmatrix(char *s, struct object matrix)
{
	fprintf(stderr,"%s %f %f %f %f %f %f\n",s,E(matrix,0),E(matrix,1),E(matrix,2),E(matrix,3),E(matrix,4),E(matrix,5));
}
void
cachestatusOP(void)
{
	push(makeint(bsize));
	push(makeint(bmax));
	push(makeint(msize));
	push(makeint(mmax));
	push(makeint(csize));
	push(makeint(cmax));
	push(makeint(blimit));
}
void
clipchar(double orx, double ory, double wid, double ht)
{
	newpathOP();
	add_element(PA_MOVETO,makepoint(orx,ory));
	add_element(PA_LINETO,add_point(makepoint(wid,0.),currentpoint()));
	add_element(PA_LINETO,add_point(makepoint(0.,-ht),currentpoint()));
	add_element(PA_LINETO,add_point(makepoint(-wid,0.),currentpoint()));
	closepathOP();
	if(do_clip())
		if(smallpath(wid/2.,ht/2.))
			Graphics.path.first = NULL;
}
int
smallpath(double wid, double ht)
{
	struct element *p;
	double pmaxx, pminx, pmaxy, pminy;
	pminy = pmaxy = Graphics.path.first->ppoint.y;
	pminx = pmaxx = Graphics.path.first->ppoint.x;
	for(p=Graphics.path.first; p != NULL;p=p->next){
		if(p->ppoint.x < pminx)pminx = p->ppoint.x;
		else if(p->ppoint.x > pmaxx)pmaxx = p->ppoint.x;
		if(p->ppoint.y < pminy)pminy = p->ppoint.y;
		else if(p->ppoint.y > pmaxy)pmaxy = p->ppoint.y;
	}
	if(pmaxx - pminx < wid)return(1);
	if(pmaxy - pminy < ht)return(1);
	return(0);
}
#ifdef Plan9
Bitmap *
cachealloc(Rectangle r)
{
	Bitmap *bx;
	bx=balloc(r,0);
	return(bx);
}
#endif
