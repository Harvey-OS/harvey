/*% cyntax % -lfb && cc -go # % -lfb
 * utah -- convert a utah format picture into a picio file.
 *	doesn't do enough error checking.
 * A utah picture contains:
 * size		data
 * ----		----
 * 2		magic number
 * 2		x position
 * 2		y position
 * 2		x size
 * 2		y size
 * 1		flags
 * 1		# of channels
 * 1		# of bits/channel (this program supports only 8 bits/channel)
 * 1		# of color map channels
 * 1		log2 of # of color map entries
 * ncolor	background color (padded to even length)
 * 2*ncmap*(1<<cmaplen)
 *		color map entries
 * 0 or 2	length of comment (only if flags has COMMENT set)
 * ncomment	comment data
 * indefinite	coded image data (see rdimage)	
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <stdio.h>
#include <fb.h>
struct utah{
	short magic;			/* 0146122, all integers are filed lsb first */
	short xpos, ypos;		/* coordinates of upper-left corner */
	short xsize, ysize;		/* width, height */
	unsigned char flags;		/* see definitions below */
	unsigned char ncolors;		/* # of channels, not counting alpha */
	char pixelbits;			/* # of bits/channel, must be 8 */
	unsigned char ncmap;		/* # of channels of color map */
	unsigned char cmaplen;		/* 1<<cmaplen is # of 2-byte color map entries */
	unsigned char *bg;		/* background color, if NOBG not set in flags */
	short *cmap;			/* color map data */
	short ncomment;			/* length of text annotation */
	char *comment;			/* comment data */
	unsigned char *image;		/* image data */
}utah;
#define	MAGIC	(short)0146122
/*
 * flags
 */
#define	CLR	1		/* clear the image before reading runs */
#define	NOBG	2		/* no background stored */
#define	ALPHA	4		/* image contains alpha channel */
#define	COMMENT	8		/* image contains text annotation */
/*
 * opcodes
 */
#define	SKIPLINES	1
#define	SETCOLOR	2
#define	SKIPPIXELS	3
#define	PIXELDATA	5
#define	RUN		6
#define	ENDFILE		7
#define	SHORT		0
#define	LONG		0x40
#define	LENGTH		0xC0
int rdshort(FILE *f){
	int c0, c1;
	c0=getc(f);
	c1=getc(f);
	if(c0==EOF || c1==EOF){
		fprintf(stderr, "utah: EOF!\n");
		exits("eof");
	}
	if(c1&128) c1|=~255;	/* sign extension */
	return c0|(c1<<8);
}
void clear(void){
	unsigned char *p=utah.image;
	unsigned char *q=utah.image+utah.xsize*utah.ysize*utah.ncolors;
	unsigned char *r;
	unsigned char *s=utah.bg+utah.ncolors;
	while(p!=q){
		r=utah.bg;
		while(r!=s) *p++=*r++;
	}
}
FILE *rdhdr(char *file){
	FILE *f=fopen(file, "r");
	if(f==0){
		perror(file);
		exits("can't open input");
	}
	utah.magic=rdshort(f);
	if(utah.magic!=MAGIC){
		fprintf(stderr, "%s: bad magic %06o\n", file, utah.magic&0177777);
		exits("bad magic");
	}
	utah.xpos=rdshort(f);
	utah.ypos=rdshort(f);
	utah.xsize=rdshort(f);
	utah.ysize=rdshort(f);
	utah.flags=getc(f);
	utah.ncolors=getc(f);
	utah.pixelbits=getc(f);
	if(utah.pixelbits!=8){
		fprintf(stderr, "%s: pixelbits!=8\n", file);
		exits("not 8 bit");
	}
	utah.ncmap=getc(f);
	utah.cmaplen=getc(f);
	if(!(utah.flags&NOBG)){
		utah.bg=(unsigned char *)malloc(utah.ncolors+1);
		if(utah.bg==0){
		NoSpace:
			fprintf(stderr, "No space\n");
			exits("no space");
		}
		fread((char *)utah.bg, 1, utah.ncolors, f);
		utah.bg[utah.ncolors]=0;
	}
	if((utah.ncolors&1)==0) getc(f);
	if(utah.flags&ALPHA) utah.ncolors++;
	utah.image=(unsigned char *)malloc(utah.ncolors*utah.xsize*utah.ysize);
	if(utah.image==0) goto NoSpace;
	if(utah.ncmap){
		utah.cmap=(short *)malloc(utah.ncmap*(1<<utah.cmaplen)*2);
		if(utah.cmap==0) goto NoSpace;
		fread((char *)utah.cmap, 2, utah.ncmap*(1<<utah.cmaplen), f);
	}
	if(utah.flags&COMMENT){
		utah.ncomment=rdshort(f);
		utah.comment=malloc(utah.ncomment);
		if(utah.comment==0) goto NoSpace;
		fread(utah.comment, 1, utah.ncomment, f);
		if(utah.ncomment&1) getc(f);
	}
	if((utah.flags&(CLR|NOBG))==CLR)
		clear();
	return f;
}
#define	setpixp() (pixp=&utah.image[(line*utah.xsize+pixel)*utah.ncolors+channel])
int rdimage(FILE *f){
	unsigned char *pixp;
	int i, op, channel=0, line=0, pixel=0, value, length;
	while((op=getc(f))!=EOF){
		if((op&LENGTH)==LONG) getc(f);
		switch(op){
		default:
			fprintf(stderr, "Illegal opcode %o\n", op);
			exits("bad opcode");
		case SHORT|SKIPLINES:
			line+=getc(f);
			pixel=0;
			break;
		case LONG|SKIPLINES:
			line+=rdshort(f);
			pixel=0;
			break;
		case SHORT|SETCOLOR:
			channel=getc(f);
			if(channel==255) channel=utah.ncolors-1;
			pixel=0;
			break;
		case LONG|SETCOLOR:
			channel=rdshort(f);
			if(channel==255) channel=utah.ncolors-1;
			pixel=0;
			break;
		case SHORT|SKIPPIXELS:
			pixel+=getc(f);
			break;
		case LONG|SKIPPIXELS:
			pixel+=rdshort(f);
			break;
		case SHORT|PIXELDATA:
			length=getc(f)+1;
			goto PixData;
		case LONG|PIXELDATA:
			length=rdshort(f)+1;
		PixData:
			setpixp();
			for(i=0;i!=length;i++){
				*pixp=getc(f);
				pixp+=utah.ncolors;
			}
			pixel+=length;
			if(length&1) getc(f);
			break;
		case SHORT|RUN:
			length=getc(f)+1;
			goto Run;
		case LONG|RUN:
			length=rdshort(f)+1;
		Run:
			value=getc(f);
			getc(f);
			setpixp();
			for(i=0;i!=length;i++){
				*pixp=value;
				pixp+=utah.ncolors;
			}
			pixel+=length;
			break;
		case SHORT|ENDFILE:
			getc(f);
		case LONG|ENDFILE:
			return 1;
		}
	}
	return 0;
}
char *chan[]={
	0,
	"m",
	"ma",
	"rgb",
	"rgba",
	"mz...",
	"maz...",
	"rgbz...",
	"rgbaz..."
};
void output(void){
	PICFILE *f;
	int i;
	if(utah.ncolors<=0 || 8<=utah.ncolors){
		fprintf(stderr, "bad ncolors\n");
		exits("bad ncolors");
	}
	f=picopen_w("OUT", "runcode", utah.xpos, utah.ypos, utah.xsize, utah.ysize,
		chan[utah.ncolors], 0, 0);
	if(f==0){
		picerror("utah2pic");
		exits("can't open output");
	}
	for(i=utah.ysize-1;i>=0;--i)
		picwrite(f, utah.image+i*utah.xsize*utah.ncolors);
}
void main(int argc, char *argv[]){
	if(getflags(argc, argv, "")!=2) usage("file");
	rdimage(rdhdr(argv[1]));
	output();
	exits("");
}
