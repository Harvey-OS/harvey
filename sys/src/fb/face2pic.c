/*
 * Convert face-save pictures to picfiles
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
#include <stdio.h>
int gethex(void){
	int c0, c1;
	do{
		c0=getchar();
		if(c0==EOF){
			fprintf(stderr, "face2pic: premature eof\n");
			exits("eof");
		}
	}while(!strchr("0123456789ABCDEF", c0));
	c0-='0'<=c0 && c0<='9'?'0':'A'-10;
	do{
		c1=getchar();
		if(c1==EOF){
			fprintf(stderr, "face2pic: premature eof\n");
			exits("eof");
		}
	}while(!strchr("0123456789ABCDEF", c1));
	c1-='0'<=c1 && c1<='9'?'0':'A'-10;
	return (c0<<4)|c1;
}
main(int argc, char *argv[]){
	char line[1000];
	char *image;
	int gotdim=0, wid, hgt, depth, x, y;
	PICFILE *out;
	switch(argc){
	default:
		fprintf(stderr, "Usage: %s [face-saver-file]\n", argv[0]);
		exits("usage");
	case 2:
		if(freopen(argv[1], "r", stdin)==0){
			perror(argv[1]);
			exits("can't open input");
		}
		break;
	case 1:
		break;
	}
	while(fgets(line, sizeof line, stdin)){
		if(strchr(line, ':')==0) break;
		if(strncmp(line, "PicData:", 8)==0){
			if(sscanf(line, "PicData: %d %d %d", &wid, &hgt, &depth)!=3){
				fprintf(stderr, "%s: format error (%s)\n", argv[0], line);
				exits("fmt err");
			}
			gotdim++;
		}
	}
	if(!gotdim){
		fprintf(stderr, "%s: no Picdata\n", argv[0]);
		exits("fmt err");
	}
	image=malloc(wid*hgt);
	if(image==0){
		fprintf(stderr, "%s: can't malloc\n", argv[0]);
		exits("no space");
	}
	out=picopen_w("OUT", "dump", 0, 0, 48, 48, "m", argv, 0);
	if(out==0){
		picerror(argv[0]);
		exits("can't create");
	}
	for(y=0;y!=hgt;y++) for(x=0;x!=wid;x++) image[y*wid+x]=gethex();
	for(y=hgt-1;y>=0;--y) picwrite(out, image+y*wid);
	exits("");
}
