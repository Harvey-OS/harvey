/*
 * Display pages of unicode character set
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#define	SIZE	24
#define	NROW	16
#define	NCOL	16
#define	GAP	16
typedef struct Page Page;
struct Page{
	Rectangle r;
	int first, step;
};
Page whole, detail;
int snarf;
/*
 * The following table is abstracted from
 *	The Unicode Standard
 *	Worldwide Character Encoding
 *	Version 1.0, Volume 1
 */
char *pagename[256]={
[0x00] "Control/ASCII/Control/Latin1",
[0x01] "European Latin/Extended Latin",
[0x02] "Standard Phonetic/Modifier Letters",
[0x03] "Generic Diacritical Marks/Greek",
[0x04] "Cyrillic",
[0x05] "Armenian/Hebrew",
[0x06] "Arabic",
[0x07] "Unassigned, Reserved for Right-To-Left Characters",
[0x08] "Unassigned, Reserved for Right-To-Left Characters",
[0x09] "Devanagari/Bengali",
[0x0a] "Gurmukhi/Gujarati",
[0x0b] "Oriya/Tamil",
[0x0c] "Telugu/Kannada",
[0x0d] "Malayalam",
[0x0e] "Thai/Lao",
[0x10] "Tibetan/Georgian",
[0x20] "General Punctuation/Superscripts and Subscripts/Currency Symbols/Diacritical Marks for Symbols",
[0x21] "Letterlike Symbols/Number Forms/Arrows",
[0x22] "Mathematical Operators",
[0x23] "Miscellaneous Technical",
[0x24] "Pictures for Control Codes/Optical Character Recognition/Enclosed Alphanumerics",
[0x25] "Form and Chart Components/Blocks/Geometric Shapes",
[0x26] "Miscellaneous Dingbats",
[0x27] "Zapf Dingbats",
[0x30] "CJK Symbols and Punctuation/Hiragana/Katakana",
[0x31] "Bopomofo/Hangul Elements/CJK Miscellaneous",
[0x32] "Enclosed CJK Letters and Ideographs",
[0x33] "CJK Squared Words/CJK Squared Abbreviations",
[0x34] "Korean Hangul Syllables",
[0x35] "Korean Hangul Syllables",
[0x36] "Korean Hangul Syllables",
[0x37] "Korean Hangul Syllables",
[0x38] "Korean Hangul Syllables",
[0x39] "Korean Hangul Syllables",
[0x3a] "Korean Hangul Syllables",
[0x3b] "Korean Hangul Syllables",
[0x3c] "Korean Hangul Syllables",
[0x3d] "Korean Hangul Syllables",
[0x4e] "CJK Ideographs",
[0x4f] "CJK Ideographs",
[0x50] "CJK Ideographs",
[0x51] "CJK Ideographs",
[0x52] "CJK Ideographs",
[0x53] "CJK Ideographs",
[0x54] "CJK Ideographs",
[0x55] "CJK Ideographs",
[0x56] "CJK Ideographs",
[0x57] "CJK Ideographs",
[0x58] "CJK Ideographs",
[0x59] "CJK Ideographs",
[0x5a] "CJK Ideographs",
[0x5b] "CJK Ideographs",
[0x5c] "CJK Ideographs",
[0x5d] "CJK Ideographs",
[0x5e] "CJK Ideographs",
[0x5f] "CJK Ideographs",
[0x60] "CJK Ideographs",
[0x61] "CJK Ideographs",
[0x62] "CJK Ideographs",
[0x63] "CJK Ideographs",
[0x64] "CJK Ideographs",
[0x65] "CJK Ideographs",
[0x66] "CJK Ideographs",
[0x67] "CJK Ideographs",
[0x68] "CJK Ideographs",
[0x69] "CJK Ideographs",
[0x6a] "CJK Ideographs",
[0x6b] "CJK Ideographs",
[0x6c] "CJK Ideographs",
[0x6d] "CJK Ideographs",
[0x6e] "CJK Ideographs",
[0x6f] "CJK Ideographs",
[0x70] "CJK Ideographs",
[0x71] "CJK Ideographs",
[0x72] "CJK Ideographs",
[0x73] "CJK Ideographs",
[0x74] "CJK Ideographs",
[0x75] "CJK Ideographs",
[0x76] "CJK Ideographs",
[0x77] "CJK Ideographs",
[0x78] "CJK Ideographs",
[0x79] "CJK Ideographs",
[0x7a] "CJK Ideographs",
[0x7b] "CJK Ideographs",
[0x7c] "CJK Ideographs",
[0x7d] "CJK Ideographs",
[0x7e] "CJK Ideographs",
[0x7f] "CJK Ideographs",
[0x80] "CJK Ideographs",
[0x81] "CJK Ideographs",
[0x82] "CJK Ideographs",
[0x83] "CJK Ideographs",
[0x84] "CJK Ideographs",
[0x85] "CJK Ideographs",
[0x86] "CJK Ideographs",
[0x87] "CJK Ideographs",
[0x88] "CJK Ideographs",
[0x89] "CJK Ideographs",
[0x8a] "CJK Ideographs",
[0x8b] "CJK Ideographs",
[0x8c] "CJK Ideographs",
[0x8d] "CJK Ideographs",
[0x8e] "CJK Ideographs",
[0x8f] "CJK Ideographs",
[0x90] "CJK Ideographs",
[0x91] "CJK Ideographs",
[0x92] "CJK Ideographs",
[0x93] "CJK Ideographs",
[0x94] "CJK Ideographs",
[0x95] "CJK Ideographs",
[0x96] "CJK Ideographs",
[0x97] "CJK Ideographs",
[0x98] "CJK Ideographs",
[0x99] "CJK Ideographs",
[0x9a] "CJK Ideographs",
[0x9b] "CJK Ideographs",
[0x9c] "CJK Ideographs",
[0x9d] "CJK Ideographs",
[0x9e] "CJK Ideographs",
[0x9f] "CJK Ideographs",
[0xe8] "Private Use Area",
[0xe9] "Private Use Area",
[0xea] "Private Use Area",
[0xeb] "Private Use Area",
[0xec] "Private Use Area",
[0xed] "Private Use Area",
[0xee] "Private Use Area",
[0xef] "Private Use Area",
[0xf0] "Private Use Area",
[0xf1] "Private Use Area",
[0xf2] "Private Use Area",
[0xf3] "Private Use Area",
[0xf4] "Private Use Area",
[0xf5] "Private Use Area",
[0xf6] "Private Use Area",
[0xf7] "Private Use Area",
[0xf8] "Private Use Area",
[0xf9] "Private Use Area",
[0xfa] "Private Use Area",
[0xfb] "Private Use Area",
[0xfc] "Private Use Area",
[0xfd] "Private Use Area",
[0xfe] "CNS 11643 Compatibility/Small Variants/Basic Glyphs for Arabic Language",
[0xff] "Halfwidth and Fullwidth Variants/Special",
};
void showsnarf(void);
void seepage(Page *pg){
	Point p;
	int c;
	char buf[10];
	bitblt(&screen, sub(pg->r.min, Pt(2, 2)), &screen, inset(pg->r, -2), Zero);
	for(p.x=0,c=pg->first;p.x!=NCOL;p.x++){
		for(p.y=0;p.y!=NROW;p.y++,c+=pg->step){
			sprint(buf, "%C", c);
			string(&screen, add(pg->r.min, mul(p, SIZE)), font, buf, S);
		}
		buf[0]="0123456789abcdef"[p.x];
		buf[1]='\0';
		string(&screen, add(pg->r.min, mul(Pt(p.x, NCOL), SIZE)), font, buf, S);
		string(&screen, add(pg->r.min, mul(Pt(NCOL, p.x), SIZE)), font, buf, S);
	}
	bflush();
}
void ereshaped(Rectangle r){
	screen.r=r;
	screen.clipr=inset(screen.r, 2);
	bitblt(&screen, screen.clipr.min, &screen, screen.clipr, Zero);
	whole.r.min=sub(div(add(r.min, r.max), 2), Pt((NCOL+1)*SIZE+GAP, (NROW+1)*SIZE/2));
	whole.r.max=add(whole.r.min, Pt((NCOL+1)*SIZE, (NROW+1)*SIZE));
	whole.first=0;
	whole.step=NROW*NCOL;
	detail.r.min=Pt(whole.r.max.x+2*GAP, whole.r.min.y);
	detail.r.max=add(detail.r.min, Pt((NCOL+1)*SIZE, (NROW+1)*SIZE));
	/* don't set detail.first, so we see the same page as before */
	detail.step=1;
	seepage(&whole);
	seepage(&detail);
	showsnarf();
}
int hit(Point p, Page *pg){
	if(!ptinrect(p, pg->r)) return -1;
	p=div(sub(p, pg->r.min), SIZE);
	if(p.y>=NROW || p.x>=NCOL) return -1;
	return p.x*NROW+p.y;
}
enum{
	TRUNC,
	EXIT
};
char *item3[]={
	[TRUNC] "empty snarf",
	[EXIT]  "exit",
	0
};
Menu menu3={item3};
Cursor confirmcurs={
	0, 0,
	0x0F, 0xBF, 0x0F, 0xBF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFE, 0xFF, 0xFE,
	0xFF, 0xFE, 0xFF, 0xFF, 0x00, 0x03, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFC,

	0x00, 0x0E, 0x07, 0x1F, 0x03, 0x17, 0x73, 0x6F,
	0xFB, 0xCE, 0xDB, 0x8C, 0xDB, 0xC0, 0xFB, 0x6C,
	0x77, 0xFC, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03,
	0x94, 0xA6, 0x63, 0x3C, 0x63, 0x18, 0x94, 0x90
};
/*
 * Wait for mouse down
 */
void mdown(Mouse *mp){
	while(mp->buttons==0) *mp=emouse();
}
/*
 * Wait for mouse up
 */
void mup(Mouse *mp){
	while(mp->buttons!=0) *mp=emouse();
}
/*
 * wait for confirming click on button b
 */
int confirm(int b, Mouse *mp){
	int down;
	cursorswitch(&confirmcurs);
	mup(mp);
	mdown(mp);
	down=mp->buttons==(1<<(b-1));
	mup(mp);
	cursorswitch(0);
	return down;
}
/*
 * Return the offset of the first byte of the line containing byte offs in file fd.
 * Fill buf with the line.
 */
int line(int offs, int fd, char *buf, int nbuf){
	char *nl;
	int n, nread;
	nread=nbuf-1;		/* save space to add a nul */
	while(offs){
		if(offs<nread){
			nread=offs;
			offs=0;
		}
		else
			offs-=nread;
		seek(fd, offs, 0);
		n=read(fd, buf, nread);
		if(n<=0) return -1;		/* should never happen */
		buf[n]='\0';
		nl=strrchr(buf, '\n');
		if(nl){
			offs+=(nl-buf)+1;
			break;
		}
	}
	seek(fd, offs, 0);
	n=read(fd, buf, nbuf-1);
	if(n<=0) return -1;	/* Really(!) should never happen */
	buf[n]='\0';
	nl=strchr(buf, '\n');
	if(nl) *nl='\0';
	return offs;
}
int look(int fd, char *key, char *out, int nout){
	long lwb, mid, upb;
	Dir d;
	int nkey;
	if(nout<2) return 0;		/* need at least space for \n\0 */
	if(dirfstat(fd, &d)<0) return 0;
	nkey=strlen(key);
	lwb=line(0, fd, out, nout);
	if(lwb<0) return 0;
	switch(strncmp(out, key, nkey)){
	case -1: break;
	case 0: return 1;
	case 1: return 0;
	}
	upb=line(d.length-1, fd, out, nout);
	if(upb<0) return 0;
	switch(strncmp(out, key, nkey)){
	case -1: return 0;
	case 0: return 1;
	case 1: break;
	}
	while(lwb<=upb){
		mid=line((lwb+upb)/2, fd, out, nout);
		if(mid<0) return 0;
		switch(strncmp(out, key, nkey)){
		case -1: lwb=mid+strlen(out)+1; break;
		case  0: return 1;
		case  1: upb=mid-1; break;
		}
	}
	return 0;
}
#define	NLINE	2000
void showsnarf(void){
	char line[NLINE];
	Point p;
	int n;
	if(snarf>=0){
		strcpy(line, "snarf: ");
		seek(snarf, 0, 0);
		n=read(snarf, line+7, NLINE-8);
		if(n<0) n=0;
		line[n+7]='\0';
	}
	else
		strcpy(line, "no /dev/snarf");
	p=string(&screen, add(screen.clipr.min, Pt(4, 6+font->height)), font, line, S);
	bitblt(&screen, p, &screen, Rect(p.x, p.y, screen.clipr.max.x, p.y+font->height),
		Zero);
}
void main(int argc, char *argv[]){
	Mouse m;
	Point p;
	int h, n;
	char buf[100+NLINE];
	char line[NLINE];
	int unames;
	unames=open("/lib/unicode", OREAD);
	snarf=open("/dev/snarf", ORDWR);
	binit(0, argc>1?argv[1]:0, argv[0]);
	einit(Emouse|Ekeyboard);
	ereshaped(screen.r);
	for(;;){
		m=emouse();
		switch(m.buttons){
		case 1:
		case 2:
			h=hit(m.xy, &whole);
			if(h>=0){
				sprint(buf, "%x%xxx %s", h/NROW, h%NROW,
					pagename[h]?pagename[h]:"");
				p=string(&screen, add(screen.clipr.min, Pt(4, 4)),
					font, buf, S);
				bitblt(&screen, p, &screen,
					Rect(p.x,p.y, screen.clipr.max.x,p.y+font->height),
					Zero);
				detail.first=h*NROW*NCOL;
				seepage(&detail);
				break;
			}
			h=hit(m.xy, &detail);
			if(h<0) break;
			sprint(buf, "%x%x%x%x",
				detail.first/(NROW*NROW)/NROW,
				detail.first/(NROW*NROW)%NROW,
				h/NROW, h%NROW);
			if(unames>=0 && look(unames, buf, line, NLINE) && strlen(line)>5){
				strcat(buf, " ");
				strcat(buf, line+5);
			}
			p=string(&screen, add(screen.clipr.min, Pt(4, 4)),
				font, buf, S);
			if(p.x<screen.clipr.max.x)
				bitblt(&screen, p, &screen,
					Rect(p.x,p.y, screen.clipr.max.x,p.y+font->height),
					Zero);
			if(m.buttons!=2) break;
			if(snarf>=0){
				/* there appears to be a bug in /dev/snarf */
				seek(snarf, 0, 0);
				n=read(snarf, buf, NLINE);
				seek(snarf, 0, 0);
				write(snarf, buf, n);
				fprint(snarf, "%C", detail.first+h);
			}
			break;
		case 4:
			switch(menuhit(3, &m, &menu3)){
			case TRUNC:
				close(snarf);
				snarf=create("/dev/snarf", ORDWR, 0666);
				break;
			case EXIT:
				if(confirm(3, &m)) exits("");
				break;
			}
		}
		showsnarf();
		mup(&m);
	}
}
