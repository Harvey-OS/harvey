/*
 * convert sgi images to picfiles
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
/*
 * All values are stored MSB first
 *
 * For verbatim storage, the image follows the header, in TYPE=pico order
 * For rle storage, there first follow two tables of size ysize*zsize*sizeof long,
 * in z-major order (i.e. all the y's for a given z are together).
 * The first table contains file offsets of channel-scan-lines.  The
 * second contains lengths of line codes, in bytes.
 * Run codes are like utah rle files.  The low 7 bits of the first value are
 * a count.  If bit 8 is zero, we repeat the following value count times.
 * If bit 8 is one, we copy the next count values.  (Values are bytes or
 * shorts depending on bpc.)
 */
typedef struct SgiHead{
	short magic;		/* 474 */
	char storage;		/* 0->verbatim 1->rle */
	char bpc;		/* bytes per component, usually 1 */
	ushort dimension;	/* 1, 2, or 3 */
	ushort xsize;		/* length of scanlines, if dimension>=1 */
	ushort ysize;		/* number of scanlines, if dimension>=2 */
	ushort zsize;		/* channels per pixel, if dimension==3 */
	long pixmin;		/* smallest pixel value in image */
	long pixmax;		/* largest pixel value in image */
	char dummy[4];		/* should be zero */
	char imagename[80];	/* null-terminated, not commonly used */
	long colormap;		/* 0->normal (b/w or full color)
				 * 1->dithered (332 rgb, obsolete)
				 * 2->screen (obsolete)
				 * 3->colormap (not really an image)
				 */
	char dummy2[404];	/* should be zeros */
}SgiHead;
SgiHead hdr;
int *offset, *length;
unsigned char *inbuf;
char *chan[]={
	0,
	"m",
	0,
	"rgb",
	"rgba",
};
void readhdr(int fd){
	unsigned char hbuf[512];
	int i, j, ok, nentry, nblock, len;
	if(read(fd, hbuf, 512)!=512){
		fprint(2, "Can't read image header\n");
		exits("no header");
	}
	hdr.magic=(hbuf[0]<<8)+hbuf[1];
	hdr.storage=hbuf[2];
	hdr.bpc=hbuf[3];
	hdr.dimension=(hbuf[4]<<8)+hbuf[5];
	hdr.xsize=(hbuf[6]<<8)+hbuf[7];
	hdr.ysize=(hbuf[8]<<8)+hbuf[9];
	hdr.zsize=(hbuf[10]<<8)+hbuf[11];
	hdr.pixmin=(hbuf[12]<<24)+(hbuf[13]<<16)+(hbuf[14]<<8)+hbuf[15];
	hdr.pixmax=(hbuf[16]<<24)+(hbuf[17]<<16)+(hbuf[18]<<8)+hbuf[19];
	for(i=0;i!=4;i++) hdr.dummy[i]=hbuf[20+i];
	for(i=0;i!=80;i++) hdr.imagename[i]=hbuf[24+i];
	hdr.colormap=(hbuf[104]<<24)+(hbuf[105]<<16)+(hbuf[106]<<8)+hbuf[107];
	for(i=0;i!=404;i++) hdr.dummy2[i]=hbuf[108+i];
	ok=1;
	if(hdr.magic!=474){
		fprint(2, "bad magic %d (should be 474)\n", hdr.magic);
		ok=0;
	}
	if(hdr.storage!=0 && hdr.storage!=1){
		fprint(2, "bad storage %d (should be 0 or 1)\n", hdr.storage);
		ok=0;
	}
	switch(hdr.dimension){
	default:
		fprint(2, "bad dimension %d (should be 0, 1 or 2)\n", hdr.dimension);
		ok=0;
		break;
	case 1:
		if(hdr.ysize!=1){
			fprint(2, "ysize must be 1 (not %d) when dimension is 1\n",
				hdr.ysize);
			ok=0;
		}
	case 2:
		if(hdr.zsize!=1){
			fprint(2, "zsize must be 1 (not %d) when dimension is not 3\n",
				hdr.zsize);
			ok=0;
		}
		break;
	case 3:
		if(hdr.zsize!=1 && hdr.zsize!=3 && hdr.zsize!=4){
			fprint(2, "unsupported zsize %d (should be 1, 3 or 4)\n",
				hdr.zsize);
			ok=0;
		}
		break;
	}
	if(hdr.colormap!=0){
		fprint(2, "non-zero colormap (%d) not supported!\n", hdr.colormap);
		ok=0;
	}
	if(!ok) exits("bad hdr");
	if(hdr.storage){
		nentry=hdr.ysize*hdr.zsize;
		offset=malloc(nentry*sizeof(int));
		if(offset==0) goto NoMem;
		for(i=0;i!=nentry;i+=nblock){
			nblock=nentry-i;
			if(nblock>128) nblock=128;
			read(fd, hbuf, nblock*4);
			for(j=0;j!=nblock;j++)
				offset[i+j]=(hbuf[4*j]<<24)+(hbuf[4*j+1]<<16)
					   +(hbuf[4*j+2]<<8)+hbuf[4*j+3];
		}
		length=malloc(nentry*sizeof(int));
		if(length==0) goto NoMem;
		len=0;
		for(i=0;i!=nentry;i+=nblock){
			nblock=nentry-i;
			if(nblock>128) nblock=128;
			read(fd, hbuf, nblock*4);
			for(j=0;j!=nblock;j++){
				length[i+j]=(hbuf[4*j]<<24)+(hbuf[4*j+1]<<16)
					   +(hbuf[4*j+2]<<8)+hbuf[4*j+3];
				if(length[i+j]>len) len=length[i+j];
			}
		}
	}
	else
		len=hdr.xsize*hdr.bpc;
	inbuf=malloc(len);
	if(inbuf==0){
	NoMem:
		fprint(2, "can't malloc\n");
		exits("no mem");
	}
}
void getline(int fd, int y, unsigned char *buf){
	int x, z, l, count;
	unsigned char *bp, v;
	if(hdr.storage){	/* run code */
		for(z=0;z!=hdr.zsize;z++){
			seek(fd, offset[z*hdr.ysize+y], 0);
			l=length[z*hdr.ysize+y];
			if(read(fd, inbuf, l)!=l){
				fprint(2, "can't read, line %d component %d\n", y, z);
				exits("no read");
			}
			bp=inbuf;
			for(x=0;;){
				count=bp[hdr.bpc-1]&127;
				if(count==0) break;
				if(bp[hdr.bpc-1]&128){
					bp+=hdr.bpc;
					while(count){
						buf[x*hdr.zsize+z]=*bp;
						bp+=hdr.bpc;
						x++;
						--count;
					}
				}
				else{
					v=bp[hdr.bpc];
					while(count){
						buf[x*hdr.zsize+z]=v;
						x++;
						--count;
					}
					bp+=2*hdr.bpc;
				}
			}
		}
	}
	else{			/* dump */
		for(z=0;z!=hdr.zsize;z++){
			seek(fd, 512+(z*hdr.ysize+y)*hdr.xsize, 0);
			l=hdr.xsize*hdr.bpc;
			if(read(fd, inbuf, l)!=l){
				fprint(2, "can't read, line %d component %d\n", y, z);
				exits("no read");
			}
			for(x=0;x!=hdr.xsize;x++)
				buf[x*hdr.zsize+z]=inbuf[x*hdr.bpc];
		}
	}
}
void main(int argc, char *argv[]){
	PICFILE *out;
	int in, y;
	unsigned char *buf;
	argc=getflags(argc, argv, "n:1[name]");
	if(argc!=2) usage("file");
	in=open(argv[1], OREAD);
	if(in==-1){
		fprint(2, "%s: can't open %s\n", argv[0], argv[1]);
		exits("no open");
	}
	readhdr(in);
	buf=malloc(hdr.xsize*hdr.zsize);
	if(buf==0){
		fprint(2, "%s: can't malloc\n", argv[0]);
		exits("no mem");
	}
	out=picopen_w("OUT", "runcode", 0, 0, hdr.xsize, hdr.ysize, chan[hdr.zsize], 0, 0);
	if(flag['n']) picputprop(out, "NAME", flag['n'][0]);
	else picputprop(out, "NAME", argv[1]);
	if(out==0){
		fprint(2, "%s: can't open OUT\n", argv[0]);
		exits("no OUT");
	}
	for(y=hdr.ysize-1;y>=0;--y){
		getline(in, y, buf);
		picwrite(out, buf);
	}
	exits(0);
}
