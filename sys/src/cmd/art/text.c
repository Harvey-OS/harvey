#include "art.h"
Dpoint neartext(Item *ip, Dpoint testp){
	Point s=strsize(ip->face->font, ip->text);
	Dpoint hs, p, close;
	Flt rad, r;
	hs.x=s.x/DPI/2.;
	hs.y=s.y/DPI/2.;
	p=Dpt(ip->p[0].x-hs.x, ip->p[0].y-hs.y);   rad=dist(testp, p);      close=p;
	p=Dpt(ip->p[0].x-hs.x, ip->p[0].y     ); if((r=dist(testp, p))<rad){close=p; rad=r;}
	p=Dpt(ip->p[0].x-hs.x, ip->p[0].y+hs.y); if((r=dist(testp, p))<rad){close=p; rad=r;}
	p=Dpt(ip->p[0].x,      ip->p[0].y-hs.y); if((r=dist(testp, p))<rad){close=p; rad=r;}
	p=    ip->p[0]                         ; if((r=dist(testp, p))<rad){close=p; rad=r;}
	p=Dpt(ip->p[0].x,      ip->p[0].y+hs.y); if((r=dist(testp, p))<rad){close=p; rad=r;}
	p=Dpt(ip->p[0].x+hs.x, ip->p[0].y-hs.y); if((r=dist(testp, p))<rad){close=p; rad=r;}
	p=Dpt(ip->p[0].x+hs.x, ip->p[0].y     ); if((r=dist(testp, p))<rad){close=p; rad=r;}
	p=Dpt(ip->p[0].x+hs.x, ip->p[0].y+hs.y); if((r=dist(testp, p))<rad){close=p; rad=r;}
	return close;
}
void clrb(Bitmap *dst, Rectangle r, int color){
	static Bitmap *src=0;
	if(src==0){
		src=balloc(Rect(0,0,1,1), screen.ldepth);
		if(src==0) return;
	}
	point(src, Pt(0,0), color, S);
	texture(dst, r, src, S);
}
void drawtext(Item *ip, int bits, Bitmap *b, Fcode c){
	static Rectangle oldsr={0,0,0,0};
	static Bitmap *strb=0;
	Rectangle sr=Rpt(Pt(0,0), strsize(ip->face->font, ip->text));
	if(strb==0 || sr.max.x>oldsr.max.x || sr.max.y>oldsr.max.y){
		if(strb) bfree(strb);
		strb=balloc(sr, screen.ldepth);
		if(strb==0){
			msg("no space to draw text!");
			return;
		}
		oldsr=sr;
	}
	clrb(strb, sr, bits);
	string(strb, Pt(0,0), ip->face->font, ip->text, S&D);
	bitblt(b, sub(D2P(ip->p[0]), div(sr.max, 2)), strb, sr, c);
}
void edittext(void){
	track(moveall, 0, selection);
}
void translatetext(Item *ip, Dpoint delta){
	ip->p[0]=dadd(ip->p[0], delta);
}
void deletetext(Item *ip){
	free(ip->text);
}
void writetext(Item *ip, int f){
	fprint(f, "t %.3f %.3f %s %s\n", ip->p[0].x, ip->p[0].y, ip->face->name, ip->text);
}
void activatetext(Item *ip){
	hotpoint(ip->p[0]);	/* looks wrong to me */
}
Drectangle bboxtext(Item *ip){
	Point p=strsize(ip->face->font, ip->text);
	Dpoint h=Dpt((Flt)p.x/DPI/2., (Flt)p.y/DPI/2.);
	return Drpt(dsub(ip->p[0], h), dadd(ip->p[0], h));
}
int inboxtext(Item *ip, Drectangle r){
	return drectxrect(bboxtext(ip), r);
}
Dpoint nearverttext(Item *ip, Dpoint testp){
	return ip->nearpt;	/* cheat -- I know this has been set correctly */
}
Itemfns textfns={
	deletetext,
	writetext,
	activatetext,
	neartext,
	drawtext,
	edittext,
	translatetext,
	inboxtext,
	bboxtext,
	nearverttext,		/* should be neartext */
};
Item *addtext(Item *head, Dpoint p, Typeface *face, char *text){
	return additem(head, TEXT, 0., face, text, 0, &textfns, 1, p);
}
/*
 * Font tables.
 */
#define	NFACE	100
Typeface face[NFACE];
Typeface *eface=face;
Typeface *curface;
void setface(char *s){
	Typeface *f;
	Bitmap *b;
	char file[200];
	char *font, *style, *size;
	for(f=face;f!=eface;f++) if(strcmp(s, f->name)==0){
		curface=f;
		return;
	}
	if(eface==&face[NFACE]){
		msg("not enough typefaces");
		return;
	}
	font=strtok(s, ",");
	if(font==0) goto Bad;
	style=strtok(0, ",");
	if(style==0) goto Bad;
	size=strtok(0, ",");
	if(size==0) goto Bad;
	sprint(file, "/lib/font/bit/%s/%s.%s.font", font, style, size);
	eface->font=rdfontfile(file, screen.ldepth);
	if(eface->font==0){
	Bad:
		if(!fontp){
			fprint(2, "can't get font %s\n", s);
			exits("no font");
		}
		msg("font not available");
		return;
	}
	eface->file=strdup(file);
	sprint(file, "%s,%s,%s", font, style, size);
	eface->name=strdup(file);
	curface=eface;
	eface++;
}
