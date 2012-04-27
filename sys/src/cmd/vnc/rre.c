#include "vnc.h"
#include "vncs.h"

/*
 * rise and run length encoding, aka rre.
 *
 * the pixel contained in r are subdivided into
 * rectangles of uniform color, each of which
 * is encoded by <color, x, y, w, h>.
 *
 * use raw encoding if it's shorter.
 *
 * for compact rre, use limited size rectangles,
 * which are shorter to encode and therefor give better compression.
 *
 * hextile encoding uses rre encoding on at most 16x16 rectangles tiled
 * across and then down the screen.
 */
static int	encrre(uchar *raw, int stride, int w, int h, int back, int pixb, uchar *buf, int maxr, uchar *done, int (*eqpix)(uchar*, int, int), uchar *(putr)(uchar*, uchar*, int, int, int, int, int, int));
static int	eqpix16(uchar *raw, int p1, int p2);
static int	eqpix32(uchar *raw, int p1, int p2);
static int	eqpix8(uchar *raw, int p1, int p2);
static int	findback(uchar *raw, int stride, int w, int h, int (*eqpix)(uchar*, int, int));
static uchar*	putcorre(uchar *buf, uchar *raw, int p, int pixb, int x, int y, int w, int h);
static uchar*	putrre(uchar *buf, uchar *raw, int p, int pixb, int x, int y, int w, int h);
static void	putpix(Vnc *v, uchar *raw, int p, int pixb);
static int	hexcolors(uchar *raw, int stride, int w, int h, int (*eqpix)(uchar*, int, int), int back, int *fore);
static uchar	*puthexfore(uchar *buf, uchar*, int, int, int x, int y, int w, int h);
static uchar	*puthexcol(uchar *buf, uchar*, int, int, int x, int y, int w, int h);
static void	sendtraw(Vnc *v, uchar *raw, int pixb, int stride, int w, int h);

/*
 * default routine, no compression, just the pixels
 */
int
sendraw(Vncs *v, Rectangle r)
{
	int pixb, stride;
	uchar *raw;

	if(!rectinrect(r, v->image->r))
		sysfatal("sending bad rectangle");

	pixb = v->bpp >> 3;
	if((pixb << 3) != v->bpp)
		sysfatal("bad pixel math in sendraw");
	stride = v->image->width*sizeof(ulong);
	if(((stride / pixb) * pixb) != stride)
		sysfatal("bad pixel math in sendraw");
	stride /= pixb;

	raw = byteaddr(v->image, r.min);

	vncwrrect(v, r);
	vncwrlong(v, EncRaw);
	sendtraw(v, raw, pixb, stride, Dx(r), Dy(r));
	return 1;
}

int
countraw(Vncs*, Rectangle)
{
	return 1;
}

/*
 * grab the image for the entire rectangle,
 * then encode each tile
 */
int
sendhextile(Vncs *v, Rectangle r)
{
	uchar *(*putr)(uchar*, uchar*, int, int, int, int, int, int);
	int (*eq)(uchar*, int, int);
	uchar *raw, *buf, *done, *traw;
	int w, h, stride, pixb, pixlg, nr, bpr, back, fore;
	int sy, sx, th, tw, oback, ofore, k, nc;

	h = Dy(r);
	w = Dx(r);
	if(h == 0 || w == 0 || !rectinrect(r, v->image->r))
		sysfatal("bad rectangle %R in sendhextile %R", r, v->image->r);

	switch(v->bpp){
	case  8:	pixlg = 0;	eq = eqpix8;	break;
	case 16:	pixlg = 1;	eq = eqpix16;	break;
	case 32:	pixlg = 2;	eq = eqpix32;	break;
	default:
		sendraw(v, r);
		return 1;
	}
	pixb = 1 << pixlg;
	stride = v->image->width*sizeof(ulong);
	if(((stride >> pixlg) << pixlg) != stride){
		sendraw(v, r);
		return 1;
	}
	stride >>= pixlg;

	buf = malloc(HextileDim * HextileDim * pixb);
	done = malloc(HextileDim * HextileDim);
	if(buf == nil || done == nil){
		free(buf);
		free(done);
		sendraw(v, r);
		return 1;
	}
	raw = byteaddr(v->image, r.min);

	vncwrrect(v, r);
	vncwrlong(v, EncHextile);
	oback = -1;
	ofore = -1;
	for(sy = 0; sy < h; sy += HextileDim){
		th = h - sy;
		if(th > HextileDim)
			th = HextileDim;
		for(sx = 0; sx < w; sx += HextileDim){
			tw = w - sx;
			if(tw > HextileDim)
				tw = HextileDim;

			traw = raw + ((sy * stride + sx) << pixlg);

			back = findback(traw, stride, tw, th, eq);
			nc = hexcolors(traw, stride, tw, th, eq, back, &fore);
			k = 0;
			if(oback < 0 || !(*eq)(raw, back + ((traw - raw) >> pixlg), oback))
				k |= HextileBack;
			if(nc == 1){
				vncwrchar(v, k);
				if(k & HextileBack){
					oback = back + ((traw - raw) >> pixlg);
					putpix(v, raw, oback, pixb);
				}
				continue;
			}
			k |= HextileRects;
			if(nc == 2){
				putr = puthexfore;
				bpr = 2;
				if(ofore < 0 || !(*eq)(raw, fore + ((traw - raw) >> pixlg), ofore))
					k |= HextileFore;
			}else{
				putr = puthexcol;
				bpr = 2 + pixb;
				k |= HextileCols;
				/* stupid vnc clients smash foreground in this case */
				ofore = -1;
			}

			nr = th * tw << pixlg;
			if(k & HextileBack)
				nr -= pixb;
			if(k & HextileFore)
				nr -= pixb;
			nr /= bpr;
			memset(done, 0, HextileDim * HextileDim);
			nr = encrre(traw, stride, tw, th, back, pixb, buf, nr, done, eq, putr);
			if(nr < 0){
				vncwrchar(v, HextileRaw);
				sendtraw(v, traw, pixb, stride, tw, th);
				/* stupid vnc clients smash colors in this case */
				ofore = -1;
				oback = -1;
			}else{
				vncwrchar(v, k);
				if(k & HextileBack){
					oback = back + ((traw - raw) >> pixlg);
					putpix(v, raw, oback, pixb);
				}
				if(k & HextileFore){
					ofore = fore + ((traw - raw) >> pixlg);
					putpix(v, raw, ofore, pixb);
				}
				vncwrchar(v, nr);
				vncwrbytes(v, buf, nr * bpr);
			}
		}
	}
	free(buf);
	free(done);
	return 1;
}

int
counthextile(Vncs*, Rectangle)
{
	return 1;
}

static int
hexcolors(uchar *raw, int stride, int w, int h, int (*eqpix)(uchar*, int, int), int back, int *rfore)
{
	int s, es, sx, esx, fore;

	*rfore = -1;
	fore = -1;
	es = stride * h;
	for(s = 0; s < es; s += stride){
		esx = s + w;
		for(sx = s; sx < esx; sx++){
			if((*eqpix)(raw, back, sx))
				continue;
			if(fore < 0){
				fore = sx;
				*rfore = fore;
			}else if(!(*eqpix)(raw, fore, sx))
				return 3;
		}
	}

	if(fore < 0)
		return 1;
	return 2;
}

static uchar*
puthexcol(uchar *buf, uchar *raw, int p, int pixb, int x, int y, int w, int h)
{
	raw += p * pixb;
	while(pixb--)
		*buf++ = *raw++;
	*buf++ = (x << 4) | y;
	*buf++ = (w - 1) << 4 | (h - 1);
	return buf;
}

static uchar*
puthexfore(uchar *buf, uchar*, int, int, int x, int y, int w, int h)
{
	*buf++ = (x << 4) | y;
	*buf++ = (w - 1) << 4 | (h - 1);
	return buf;
}

static void
sendtraw(Vnc *v, uchar *raw, int pixb, int stride, int w, int h)
{
	int y;

	for(y = 0; y < h; y++)
		vncwrbytes(v, &raw[y * stride * pixb], w * pixb);
}

static int
rrerects(Rectangle r, int split)
{
	return ((Dy(r) + split - 1) / split) * ((Dx(r) + split - 1) / split);
}

enum
{
	MaxCorreDim	= 48,
	MaxRreDim	= 64,
};

int
countrre(Vncs*, Rectangle r)
{
	return rrerects(r, MaxRreDim);
}

int
countcorre(Vncs*, Rectangle r)
{
	return rrerects(r, MaxCorreDim);
}

static int
_sendrre(Vncs *v, Rectangle r, int split, int compact)
{
	uchar *raw, *buf, *done;
	int w, h, stride, pixb, pixlg, nraw, nr, bpr, back, totr;
	int (*eq)(uchar*, int, int);

	totr = 0;
	h = Dy(r);
	while(h > split){
		h = r.max.y;
		r.max.y = r.min.y + split;
		totr += _sendrre(v, r, split, compact);
		r.min.y = r.max.y;
		r.max.y = h;
		h = Dy(r);
	}
	w = Dx(r);
	while(w > split){
		w = r.max.x;
		r.max.x = r.min.x + split;
		totr += _sendrre(v, r, split, compact);
		r.min.x = r.max.x;
		r.max.x = w;
		w = Dx(r);
	}
	if(h == 0 || w == 0 || !rectinrect(r, v->image->r))
		sysfatal("bad rectangle in sendrre");

	switch(v->bpp){
	case  8:	pixlg = 0;	eq = eqpix8;	break;
	case 16:	pixlg = 1;	eq = eqpix16;	break;
	case 32:	pixlg = 2;	eq = eqpix32;	break;
	default:
		sendraw(v, r);
		return totr + 1;
	}
	pixb = 1 << pixlg;
	stride = v->image->width*sizeof(ulong);
	if(((stride >> pixlg) << pixlg) != stride){
		sendraw(v, r);
		return totr + 1;
	}
	stride >>= pixlg;

	nraw = w * pixb * h;
	buf = malloc(nraw);
	done = malloc(w * h);
	if(buf == nil || done == nil){
		free(buf);
		free(done);
		sendraw(v, r);
		return totr + 1;
	}
	memset(done, 0, w * h);

	raw = byteaddr(v->image, r.min);

	if(compact)
		bpr = 4 * 1 + pixb;
	else
		bpr = 4 * 2 + pixb;
	nr = (nraw - 4 - pixb) / bpr;
	back = findback(raw, stride, w, h, eq);
	if(compact)
		nr = encrre(raw, stride, w, h, back, pixb, buf, nr, done, eq, putcorre);
	else
		nr = encrre(raw, stride, w, h, back, pixb, buf, nr, done, eq, putrre);
	if(nr < 0){
		vncwrrect(v, r);
		vncwrlong(v, EncRaw);
		sendtraw(v, raw, pixb, stride, w, h);
	}else{
		vncwrrect(v, r);
		if(compact)
			vncwrlong(v, EncCorre);
		else
			vncwrlong(v, EncRre);
		vncwrlong(v, nr);
		putpix(v, raw, back, pixb);
		vncwrbytes(v, buf, nr * bpr);
	}
	free(buf);
	free(done);

	return totr + 1;
}

int
sendrre(Vncs *v, Rectangle r)
{
	return _sendrre(v, r, MaxRreDim, 0);
}

int
sendcorre(Vncs *v, Rectangle r)
{
	return _sendrre(v, r, MaxCorreDim, 1);
}

static int
encrre(uchar *raw, int stride, int w, int h, int back, int pixb, uchar *buf,
	int maxr, uchar *done, int (*eqpix)(uchar*, int, int),
	uchar *(*putr)(uchar*, uchar*, int, int, int, int, int, int))
{
	int s, es, sx, esx, sy, syx, esyx, rh, rw, y, nr, dsy, dp;

	es = stride * h;
	y = 0;
	nr = 0;
	dp = 0;
	for(s = 0; s < es; s += stride){
		esx = s + w;
		for(sx = s; sx < esx; ){
			rw = done[dp];
			if(rw){
				sx += rw;
				dp += rw;
				continue;
			}
			if((*eqpix)(raw, back, sx)){
				sx++;
				dp++;
				continue;
			}

			if(nr >= maxr)
				return -1;

			/*
			 * find the tallest maximally wide uniform colored rectangle
			 * with p at the upper left.
			 * this isn't an optimal parse, but it's pretty good for text
			 */
			rw = esx - sx;
			rh = 0;
			for(sy = sx; sy < es; sy += stride){
				if(!(*eqpix)(raw, sx, sy))
					break;
				esyx = sy + rw;
				for(syx = sy + 1; syx < esyx; syx++){
					if(!(*eqpix)(raw, sx, syx)){
						if(sy == sx)
							break;
						goto breakout;
					}
				}
				if(sy == sx)
					rw = syx - sy;
				rh++;
			}
		breakout:;

			nr++;
			buf = (*putr)(buf, raw, sx, pixb, sx - s, y, rw, rh);

			/*
			 * mark all pixels done
			 */
			dsy = dp;
			while(rh--){
				esyx = dsy + rw;
				for(syx = dsy; syx < esyx; syx++)
					done[syx] = esyx - syx;
				dsy += w;
			}

			sx += rw;
			dp += rw;
		}
		y++;
	}
	return nr;
}

/*
 * estimate the background color
 * by finding the most frequent character in a small sample
 */
static int
findback(uchar *raw, int stride, int w, int h, int (*eqpix)(uchar*, int, int))
{
	enum{
		NCol = 6,
		NExamine = 4
	};
	int ccount[NCol], col[NCol], i, wstep, hstep, x, y, pix, c, max, maxc;

	wstep = w / NExamine;
	if(wstep < 1)
		wstep = 1;
	hstep = h / NExamine;
	if(hstep < 1)
		hstep = 1;

	for(i = 0; i< NCol; i++)
		ccount[i] = 0;
	for(y = 0; y < h; y += hstep){
		for(x = 0; x < w; x += wstep){
			pix = y * stride + x;
			for(i = 0; i < NCol; i++){
				if(ccount[i] == 0){
					ccount[i] = 1;
					col[i] = pix;
					break;
				}
				if((*eqpix)(raw, pix, col[i])){
					ccount[i]++;
					break;
				}
			}
		}
	}
	maxc = ccount[0];
	max = 0;
	for(i = 1; i < NCol; i++){
		c = ccount[i];
		if(!c)
			break;
		if(c > maxc){
			max = i;
			maxc = c;
		}
	}
	return col[max];
}

static uchar*
putrre(uchar *buf, uchar *raw, int p, int pixb, int x, int y, int w, int h)
{
	raw += p * pixb;
	while(pixb--)
		*buf++ = *raw++;
	*buf++ = x >> 8;
	*buf++ = x;
	*buf++ = y >> 8;
	*buf++ = y;
	*buf++ = w >> 8;
	*buf++ = w;
	*buf++ = h >> 8;
	*buf++ = h;
	return buf;
}

static uchar*
putcorre(uchar *buf, uchar *raw, int p, int pixb, int x, int y, int w, int h)
{
	raw += p * pixb;
	while(pixb--)
		*buf++ = *raw++;
	*buf++ = x;
	*buf++ = y;
	*buf++ = w;
	*buf++ = h;
	return buf;
}

static int
eqpix8(uchar *raw, int p1, int p2)
{
	return raw[p1] == raw[p2];
}

static int
eqpix16(uchar *raw, int p1, int p2)
{
	return ((ushort*)raw)[p1] == ((ushort*)raw)[p2];
}

static int
eqpix32(uchar *raw, int p1, int p2)
{
	return ((ulong*)raw)[p1] == ((ulong*)raw)[p2];
}

static void
putpix(Vnc *v, uchar *raw, int p, int pixb)
{
	vncwrbytes(v, raw + p * pixb, pixb);
}
