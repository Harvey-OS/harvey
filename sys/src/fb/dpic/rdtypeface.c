#include "ext.h"
static int fd;
#define	NBUF	6144
static unsigned char buf[NBUF];
static unsigned char *bufp, *ebuf;
static int nextc(void){
	if(bufp==ebuf){
		int n=read(fd, buf, NBUF);
		if(n<=0) return EOF;
		bufp=buf;
		ebuf=bufp+n;
	}
	return *bufp++;
}
static int getshort(int *p){
	int c0, c1, v;
	if((c0=nextc())==EOF || (c1=nextc())==EOF)
		return 0;
	v=(c0&255)|((c1&255)<<8);
	if(v>32767) v-=65536;	/* cray, twos complement */
	*p=v;
	return 1;
}
#define	NBOUNDARY	100
Typeface *rdtypeface(char *name){
	int glyphno, n, x, y, nglyph=0, nboundary=0, i;
	Glyph glyph[NCHR];
	Boundary boundary[NBOUNDARY];
	Typeface *face;
	fd=open(name, OREAD);
	if(fd==-1){
		perror(name);
		exits("face open");
	}
	for(i=0;i!=NCHR;i++){
		glyph[i].nboundary=0;
		glyph[i].boundary=0;
	}
	while(getshort(&n)) switch(n&CMD){
	case CHR:
		if(nboundary){
			if(nglyph==0){
			NoGlyph:
				fprint(2, "%s: boundarys but no character\n", name);
				exits("face noglyph");
			}
			if(glyph[glyphno].boundary){
			Dup:
				fprint(2, "%s: duplicate character %d\n",
					name, glyphno);
				exits("face dup");
			}
			glyph[glyphno].nboundary=nboundary;
			glyph[glyphno].boundary=emalloc(nboundary*sizeof(Boundary));
			memcpy(glyph[glyphno].boundary, boundary, nboundary*sizeof(Boundary));
		}
		nboundary=0;
		glyphno=n&~CMD;
		if(glyphno>=nglyph) nglyph=glyphno+1;
		break;
	case BOUNDARY:
		if(++nboundary>NBOUNDARY){
			fprint(2, "%s: NBOUNDARY too small\n", name);
			exits("face oflo");
		}
		n&=~CMD;
		boundary[nboundary-1].npt=n;
		boundary[nboundary-1].pt=emalloc(n*sizeof(Point));
		for(i=0;i!=n;i++){
			if(!getshort(&boundary[nboundary-1].pt[i].x)
			|| !getshort(&boundary[nboundary-1].pt[i].y)){
				fprint(2, "%s: eof\n", name);
				exits("face eof");
			}
		}
		break;
	}
	if(nboundary){
		if(nglyph==0) goto NoGlyph;
		if(glyph[glyphno].boundary) goto Dup;
		glyph[glyphno].nboundary=nboundary;
		glyph[glyphno].boundary=emalloc(nboundary*sizeof(Boundary));
		memcpy(glyph[glyphno].boundary, boundary, nboundary*sizeof(Boundary));
	}
	if(nglyph==0){
		fprint(2, "%s: no characters in face\n", name);
		exits("face empty");
	}
	face=emalloc(sizeof(Typeface));
	face->nglyph=nglyph;
	face->glyph=emalloc(nglyph*sizeof(Glyph));
	memcpy(face->glyph, glyph, nglyph*sizeof(Glyph));
	close(fd);
	return face;
}
void freetypeface(Typeface *f){
	Glyph *g, *eg;
	Boundary *b, *eb;
	for(g=f->glyph;g!=eg;g++) if(g->boundary){
		eb=g->boundary+g->nboundary;
		for(b=g->boundary;b!=eb;b++) free(b->pt);
		free(g->boundary);
	}
	free(f->glyph);
	free(f);
}
/*
 * void main(int argc, char *argv[]){
 *	Typeface *f;
 *	Glyph *g, *eg;
 *	Boundary *b, *eb;
 *	Point *p, *ep;
 *	int dx, dy, i;
 *	if(argc<2){
 *		fprint(2, "Usage: %s file ...\n", argv[0]);
 *		exits("usage");
 *	}
 *	print("o\n");
 *	print("ra -150 -150 6500 6500\n");
 *	for(i=1;i!=argc;i++){
 *		f=rdtypeface(argv[i]);
 *		eg=f->glyph+f->nglyph;
 *		print("e\n");
 *		print("m 0 6000\n");
 *		print("t %s %d\n", argv[i], f->nglyph);
 *		for(g=f->glyph;g!=eg;g++) if(g->boundary){
 *			dx=(g-f->glyph)%13*500;
 *			dy=5500-(g-f->glyph)/13*500;
 *			eb=g->boundary+g->nboundary;
 *			for(b=g->boundary;b!=eb;b++){
 *				ep=b->pt+b->npt;
 *				print("m %d %d\n", ep[-1].x+dx, ep[-1].y+dy);
 *				for(p=b->pt;p!=ep;p++)
 *					print("v %d %d\n", p->x+dx, p->y+dy);
 *			}
 *		}
 *	}
 *	print("cl\n");
 * }
 */
