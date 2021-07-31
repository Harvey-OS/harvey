/*% cyntax % -lfb && cc -go # % -lfb
 *	Convert a grey-level image to postscript format
 *	D. P. Mitchell  87/12/15.
 *	Added luminance conversion for color images.  Added a flag
 *	to set width.  Also converted to local conventions.
 *	td 88.09.19
 *	Further adjusted to generate encapsulated postscript, making
 *	images includable in troff via .BP
 *	td 88.10.25
 *	tweaked for the brave new world td 89.11.02
 */
#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <libg.h>
#include <fb.h>
#define HGT	3.0	/* default height in inches, same as .BP default */
#define	PPI	72.	/* `points' per inch */
main(int argc, char *argv[]){
	PICFILE *p;
	unsigned char *bp, *ebuffer;
	int y, k, width, height, xsize, ysize, hgt;
	unsigned char buffer[8192*4];
	double scale;
	argc=getflags(argc, argv, "h:1[height]c");
	if(argc!=1 && argc!=2) usage("[picfile]");
	if(flag['h'])
		hgt=PPI*atof(flag['h'][0]);
	else
		hgt=PPI*HGT;
	p=picopen_r(argc==2?argv[1]:"IN");
	if(p==0){
		perror(argc==2?argv[1]:"IN");
		exits("open input");
	}
	width=PIC_WIDTH(p);
	height=PIC_HEIGHT(p);
	scale=(double)height/hgt;
	xsize=width/scale;
	ysize=height/scale;
	printf("%%!\n%%%%BoundingBox: 0 0 %d %d\n", xsize, ysize);
	printf("/picstr %d string def\n", width);
	printf("%d %d scale\n", xsize, ysize);
	printf("%d %d 8\n", width, height);
	printf("[ %d 0 0 %d 0 %d ]\n", width, -height, height);
	printf("{ currentfile picstr readhexstring pop }\n");
	if(flag['c']
	&& (PIC_NCHAN(p)==3 || PIC_NCHAN(p)==4 || PIC_NCHAN(p)==7 || PIC_NCHAN(p)==8))
		printf("false 3 colorimage\n");
	else
		printf("image\n");
	k=0;
	ebuffer=buffer+PIC_NCHAN(p)*width;
	for(y=0;y!=height;y++){
		picread(p, (char *)buffer);
		switch(PIC_NCHAN(p)){
		case 3:
		case 4:
		case 7:
		case 8:
			if(flag['c']){
				for(bp=buffer;bp!=ebuffer;bp+=PIC_NCHAN(p)){
					printf("%02x%02x%02x", bp[0], bp[1], bp[2]);
					if(++k%32==0)
						printf("\n");
				}
			}
			else{
				for(bp=buffer;bp!=ebuffer;bp+=PIC_NCHAN(p)){
					printf("%02x",
						(299*bp[0]+587*bp[1]+114*bp[2])/1000);
					if(++k%32==0)
						printf("\n");
				}
			}
			break;
		default:
			for(bp=buffer;bp!=ebuffer;bp+=PIC_NCHAN(p)){
				printf("%02x", bp[0]);
				if(++k%32==0)
					printf("\n");
			}
		}
	}
	if(k%32!=0)
		printf("\n");
	printf("showpage\n");
	exits("");
}
