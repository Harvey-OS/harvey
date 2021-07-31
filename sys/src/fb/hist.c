/*
 * pixel histogram
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#include <bio.h>
main(int argc, char *argv[]){
	int y;
	unsigned char *buf, *bp;
	PICFILE *in;
	int *hist, nc, nx, ny, x, c;
	Biobuf out;
	argc=getflags(argc, argv, "");
	if(argc!=1 && argc!=2) usage("[picture]");
	in=picopen_r(argc==2?argv[1]:"IN");
	if(in==0){
		perror(argc==2?argv[1]:"IN");
		exits("open input");
	}
	nc=PIC_NCHAN(in);
	nx=PIC_WIDTH(in);
	ny=PIC_HEIGHT(in);
	buf=malloc(nx*nc);
	hist=malloc(256*nc*sizeof(int));
	if(buf==0 || hist==0){
		fprint(2, "%s: can't malloc\n", argv[0]);
		exits("no mem");
	}
	Binit(&out, 1, OWRITE);
	memset(hist, 0, 256*nc*sizeof(int));
	for(y=0;y!=ny;y++){
		picread(in, buf);
		bp=buf;
		for(x=0;x!=nx;x++) for(c=0;c!=nc;c++) hist[*bp++*nc+c]++;
	}
	for(x=0;x!=256;x++)
		for(c=0;c!=nc;c++)
			Bprint(&out, "%d%c", hist[x*nc+c], c==nc-1?'\n':' ');
	Bflush(&out);
	exits(0);
}
