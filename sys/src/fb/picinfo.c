#include <u.h>
#include <libc.h>
#include <stdio.h>
char line[1024];
long siz[][2]={
	512, 480,	/* rs170 */
	512, 486,	/* nyit */
	512, 488,	/* lucasfilm */
	512, 496,	/* seen amongst gerard's picutres; allegra scanner? */
	512, 497,	/* allegra scanner */
	512, 512,	/* square */
	1280, 1024,	/* full-size Metheus */
	640, 512,	/* half-size Metheus */
	320, 256,	/* quarter-size Metheus */
	160, 128,	/* eighth-size Metheus */
	2048, 2047,	/* Eikonix scanner */
	2048, 2048,	/* Eikonix scanner */
	0, 0
};
struct{
	int nchan;
	char *kind;
}dep[]={
	1,	"B/W",
	2,	"B/W+alpha",
	3,	"RGB",
	4,	"RGBA",
	7,	"RGBZ",
	8,	"RGBAZ",
	0
};
int garbled(char *s){
	while(*s){
		if(128<=(*s&255) && *s!='\t' && *s!='\n')
			return 1;
		s++;
	}
	return 0;
}
void printbtl(FILE *f, char *type){
	print("%s", type);
	while(fgets(line, sizeof line, f)!=NULL && strcmp(line, "\n")!=0){
		if(garbled(line))
			print("\tGarbled header %s\n", line);
		else
			print("\t%s", line);
	}
}
void printdump(long size){
	int s, d, or=0;
	if(size==0){
		print("null file\n");
		return;
	}
	/* canned sizes */
	for(s=0;siz[s][0];s++) for(d=0;dep[d].nchan;d++)
		if(siz[s][0]*siz[s][1]*dep[d].nchan==size){
			if(or)
				print("or ");
			else
				or++;
			print("%ldx%ld %s ", siz[s][0], siz[s][1], dep[d].kind);
		}
	if(!or){
		/* square? */
		for(d=0;dep[d].nchan;d++){
			s=sqrt((double)(size/dep[d].nchan));
			if((long)s*s*dep[d].nchan==size){
				if(or)
					print("or ");
				else
					or++;
				print("%dx%d %s ", s, s, dep[d].kind);
			}
		}
	}
	if(or)
		print("dump\n");
	else
		print("%d byte dump (maybe)\n", size);
}
main(int argc, char *argv[]){
	FILE *f;
	int i;
	char *err="";
	if(argc==1){
		fprint(2, "Usage: picinfo pic ...\n");
		exits("usage");
	}
	for(i=1;i!=argc;i++){
		if((f=fopen(argv[i], "r"))==NULL){
			err="can't open";
			perror(argv[i]);
			continue;
		}
		if(argc!=2){
			print("%s:\t", argv[i]);
		}
		if(fgets(line, sizeof line, f)!=0
		&& strncmp(line, "TYPE=", 5)==0)
			printbtl(f, line);
		else{
			fseek(f, 0L, 2);
			printdump(ftell(f));
		}
		fclose(f);
	}
	exits(err);
}
