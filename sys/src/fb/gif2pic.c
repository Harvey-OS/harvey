/*
 * gif2pic.c - convert Compuserve GIF images into picfiles
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
typedef struct Header Header;
typedef struct Image Image;
typedef unsigned char byte;
struct Header{
	char vers[3];
	int wid;
	int hgt;
	int flags;
	int bg;
	int par;
	byte cmap[256][3];
	int ncmap;
};
#define	CMAP	0x80		/* color map follows */
#define	NBIT	0x70		/* 1 less than # bits/pixel */
#define	GSORT	0x08		/* global map is sorted */
#define	NCMAP	0x07		/* 1 less than # bits/colormap */
struct Image{
	int left, top;
	int wid, hgt;
	int flags;
	int ncmap;
	byte cmap[256][3];
	int x, y;
	int dy;
	int pass;
	byte *image;
};
Header header;
Image image;
#define	LACE	0x40		/* image is interlaced */
#define	LSORT	0x20		/* local map is sorted */
#define	EXTENSION	0x21
#define	TRAILER		0x3b
#define	IMAGE		0x2c
#define	NBUF	8192
char *cmd;
int fd;
byte buf[NBUF];
byte *bp, *ebp;
int ateof;
char *inputfile;
#define	EOF	(-1)
int nextc(void){
	int n;
	if(ateof) return EOF;
	if(bp==ebp){
		n=read(fd, buf, NBUF);
		if(n<=0){
			ateof=1;
			return EOF;
		}
		bp=buf;
		ebp=buf+n;
	}
	return *bp++;
}
int errcount;
void err(int fatal, char *msg){
	fprint(2, "%s: %s: %s\n", cmd, inputfile, msg);
	if(fatal) exits(msg);
	errcount=1;
}
int rdc(void){
	int c;
	c=nextc();
	if(c==EOF) err(1, "unexpected EOF on GIF file");
	return c;
}
int rdi(void){
	int c1, c2;
	c1=rdc();
	c2=rdc();
	return c1+c2*256;
}
void rdcmap(byte cmap[256][3], int ncmap){
	int i;
	for(i=0;i!=ncmap;i++){
		cmap[i][0]=rdc();
		cmap[i][1]=rdc();
		cmap[i][2]=rdc();
	}
}
void rdheader(void){
	if(rdc()!='G' || rdc()!='I' || rdc()!='F') err(1, "input is not a GIF file");
	header.vers[0]=rdc();
	header.vers[1]=rdc();
	header.vers[2]=rdc();
	/* should we check for a legal version? */
	header.wid=rdi();
	header.hgt=rdi();
	header.flags=rdc();
	header.bg=rdc();
	header.par=rdc();
	if(header.flags&CMAP){
		header.ncmap=2<<(header.flags&NCMAP);
		rdcmap(header.cmap, header.ncmap);
	}
	else header.ncmap=0;
}
void insertpixel(int v){
	if(image.y==-1) return;
	if(image.y>=image.hgt){
		err(0, "GIF code overflows image");
		image.y=-1;
		return;
	}
	image.image[image.y*image.wid+image.x]=v;
	++image.x;
	if(image.x==image.wid){
		image.x=0;
		image.y+=image.dy;
		if(image.y>=image.hgt && image.pass<3){
			image.y=4>>image.pass;
			image.dy=8>>image.pass;
			image.pass++;
		}
	}
}
int codesize;			/* how many bits rdcode should read */
int EOFcode;			/* what to return on end-of-data */
int nblock=0;			/* # of bytes left to read in the subblock */
/*
 * Read codesize bits from the GIF data stream.
 * The data stream is a sequence of subblocks, each
 * preceded by a byte count.  A zero count or a real EOF
 * indicates premature end of data, at which point
 * rdcode returns EOFcode, just to be civilized.
 */
int rdcode(void){
	int code, nbit, offs;
	static int remain=0;		/* bits read but not returned by rdcode */
	static int nremain=0;		/* how many bits in remain? */
	if(nblock==EOF) return EOFcode;
	nbit=codesize;
	offs=0;
	code=0;
	while(nbit>nremain){
		code|=remain<<offs;
		offs+=nremain;
		nbit-=nremain;
		if(nblock==0){
			nblock=nextc();
			if(nblock==EOF || nblock==0){
				err(0, "premature GIF block terminator");
				return EOFcode;
			}
		}
		remain=nextc();
		if(remain==EOF){
			nblock=EOF;
			err(0, "truncated GIF subblock");
			return EOFcode;
		}
		--nblock;
		nremain=8;
	}
	code|=(remain&((1<<nbit)-1))<<offs;
	remain>>=nbit;
	nremain-=nbit;
	return code;
}
/*
 * A previous version of rdlzw was based on code in gif2ras.c, and
 * in the unlikely event that it still contains code that he may lay
 * claim to, the author requires us to reproduce the following:
 *
 * gif2ras.c - Converts from a Compuserve GIF (tm) image to a Sun Raster image.
 *
 * Copyright (c) 1988, 1989 by Patrick J. Naughton
 *
 * Author: Patrick J. Naughton
 * naughton@wind.sun.com
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 */
void rdlzw(void){
	int clear;			/* reinitialize lzw tables when code==clear */
	int firstfree;			/* initial lzw table free pointer */
	int freecode;			/* lzw table free pointer */
	int initcodesize;		/* # of bits in code, when lzw tables clear */
	int maxcode;			/* code that causes codesize to be bumped */
	int code;			/* input code */
	int cur;			/* chain through lzw table */
	int prev;			/* last code read */
	int finchar;			/* char output for last code read */
	int maxout;			/* largest output value, must be (1<<nbit)-1 */
	static prefix[4096];		/* which code is the prefix of this code? */
	static byte suffix[4096];	/* last output value for this code */
	static byte out[1025];		/* output string buffer, should be 4096 long? */
	byte *outp;			/* output string pointer */
	initcodesize=rdc();
	clear=1<<initcodesize;
	EOFcode=clear+1;
	firstfree=clear+2;
	initcodesize++;
	nblock=0;
	maxout=image.ncmap-1;
	outp=out;
	codesize=initcodesize;
	code=rdcode();
	if(code!=clear){
		err(0, "GIF lzw data must start with clear code");
		maxcode=1<<codesize;
		freecode=firstfree;
		prev=0;
		finchar=prev;
	}
	else SET(maxcode, freecode, prev, finchar);
	for(;code!=EOFcode;code=rdcode()){
		if(code==clear){
			codesize=initcodesize;
			maxcode=1<<codesize;
			freecode=firstfree;
			prev=rdcode();
			finchar=prev&maxout;
			insertpixel(finchar);
		}
		else{
			cur=code;
			if(cur>=freecode){
				cur=prev;
				*outp++=finchar;
			}
			while(cur>maxout){
				if(outp>&out[1024]) err(1, "GIF lzw code too long!");
				*outp++=suffix[cur];
				cur=prefix[cur];
			}
			finchar=cur&maxout;
			insertpixel(finchar);
			while(outp!=out) insertpixel(*--outp);
			if(freecode!=4096){
				prefix[freecode]=prev;
				suffix[freecode]=finchar;
				prev=code;
				freecode++;
				if(freecode>=maxcode){
					if(codesize<12){
						codesize++;
						maxcode<<=1;
					}
				}
			}
		}
	}
	if(nblock!=EOF && (nblock!=0 || nextc()!=0))
		err(0, "extra data at end of GIF data block");
}
/*
 * skip past a group of data subblocks, stopping when a block terminator
 * (a zero-length subblock) appears.
 */
void skipdata(void){
	int i, n;
	while((n=rdc())!=0) for(i=0;i!=n;i++) rdc();
}
void rdimage(void){
	for(;;){
		switch(nextc()){
		default:
			err(1, "corrupt GIF file");
		case TRAILER:
		case EOF:
			err(1, "EOF searching for GIF image");
		case EXTENSION:
			rdc();
			skipdata();
			break;
		case IMAGE:
			image.left=rdi();
			image.top=rdi();
			image.wid=rdi();
			image.hgt=rdi();
			image.flags=rdc();
			if(image.flags&CMAP){
				image.ncmap=2<<(image.flags&NCMAP);
				rdcmap(image.cmap, image.ncmap);
			}
			else{
				memcpy(image.cmap, header.cmap, 256*3);
				image.ncmap=header.ncmap;
			}
			image.x=0;
			image.y=0;
			if(image.flags&LACE){
				image.dy=8;
				image.pass=0;
			}
			else{
				image.dy=1;
				image.pass=3;
			}
			if(image.image) free(image.image);
			image.image=malloc(image.wid*image.hgt);
			if(image.image==0) err(1, "can't allocate image memory");
			return;
		}
	}
}
typedef struct Remap Remap;
struct Remap{
	byte r, g, b, c;
};
int cmp(void *aa, void *bb){
	Remap *a, *b;
	a=aa;
	b=bb;
	return (a->r-b->r)*299+(a->g-b->g)*587+(a->b-b->b)*114;
}
/*
 * Shuffle the pixels and the color map to sidestep the
 * 4 magic pixel values, if possible.  Otherwise, just
 * sort the color map in luminance order.  This is just a
 * gesture towards detente with 8½ and rio.
 */
void remap(void){
	int hist[256];
	byte zero[256];
	Remap remap[256];
	byte pixmap[256];
	byte *p, *ep;
	int i, nzero;
	byte t;
	for(i=0;i!=256;i++) hist[i]=0;
	ep=image.image+image.wid*image.hgt;
	for(p=image.image;p!=ep;p++) hist[*p]++;
	nzero=0;
	for(i=0;i!=256;i++) if(hist[i]==0) zero[nzero++]=i;
	for(i=0;i!=256;i++){
		remap[i].r=image.cmap[i][0];
		remap[i].g=image.cmap[i][1];
		remap[i].b=image.cmap[i][2];
		remap[i].c=i;
	}
	if(nzero<4) qsort(remap, 256, 4, cmp);
	else{
		for(i=0;i!=4;i++){
			remap[zero[i]]=remap[i*85];
			remap[i*85].r=remap[i*85].g=remap[i*85].b=i*85;
			remap[i*85].c=zero[i];
		}
	}
	for(i=0;i!=256;i++){
		image.cmap[i][0]=remap[i].r;
		image.cmap[i][1]=remap[i].g;
		image.cmap[i][2]=remap[i].b;
		pixmap[remap[i].c]=i;
	}
	for(p=image.image;p!=ep;p++) *p=pixmap[*p];
}
void main(int argc, char *argv[]){
	PICFILE *p;
	int y, i, which;
	byte *linebuf, *elinebuf, *rgbp, *mp, *cmp;
	argc=getflags(argc, argv, "ms:1[image #]n:1[name]");
	switch(argc){
	default: usage("[file]");
	case 1: fd=0; inputfile="*stdin*"; break;
	case 2:
		inputfile=argv[1];
		fd=open(inputfile, OREAD);
		if(fd==-1){
			fprint(2, "%s: %s: %r\n", argv[0], inputfile);
			exits("no open");
		}
		break;
	}
	cmd=argv[0];
	which=flag['s']?atoi(flag['s'][0]):1;
	rdheader();
	for(i=1;i!=which;i++){
		rdimage();
		skipdata();
	}
	rdimage();
	rdlzw();
	if(image.ncmap==0) flag['m']=flagset;
	if(!flag['m']) image.ncmap=0;
	if(image.ncmap) remap();
	p=picopen_w("OUT", "runcode", image.left, image.top, image.wid, image.hgt,
		flag['m']?"m":"rgb", argv, image.ncmap?(char *)image.cmap:0);
	if(flag['n']) picputprop(p, "NAME", flag['n'][0]);
	else if(argc>1) picputprop(p, "NAME", argv[1]);
	if(errcount) picputprop(p, "COMMENT", "created from corrupt GIF input");
	if(p==0){
		perror(argv[1]);
		exits("can't create OUT");
	}
	if(flag['m']){
		for(y=0;y!=image.hgt;y++)
			picwrite(p, (char *)image.image+y*image.wid);
	}
	else{
		linebuf=malloc(3*image.wid);
		if(linebuf==0) err(1, "can't allocate scan-line buffer");
		elinebuf=linebuf+3*image.wid;
		for(y=0;y!=image.hgt;y++){
			for(rgbp=linebuf,mp=image.image+y*image.wid;rgbp!=elinebuf;mp++){
				cmp=image.cmap[*mp];
				*rgbp++=*cmp++;
				*rgbp++=*cmp++;
				*rgbp++=*cmp;
			}
			picwrite(p, (char *)linebuf);
		}
	}
	exits(errcount?"possibly bad output":0); 
}
