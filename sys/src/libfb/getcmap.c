/*
 * int getcmap(char *file, unsigned char *buf)
 *	Read a colormap from the given file into the buffer.
 *	Returns 1 on success, 0 otherwise.
 *	Goes to unglaublich length to figure out what the file name means:
 *	If the name is "fb", reads the colormap out of the frambuffer
 *		(binit must have been called for this to work!)
 *	If the name is "gamma",returns gamma=2.3 colormap
 *	If the name is "gamma###", returns gamma=#### colormap
 *	Looks for the file in a list of directories (given below).
 *	If the file is a picture file, extracts a colormap from it
 *	If the file is 3*256 bytes long, it's a raw colormap
 *	If the file is 256 bytes long, it's a monochrome compensation table
 */
#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
char *cmapdir[]={
	"",
	"/lib/fb/cmap/",
	0
};
getcmap(char *f, unsigned char *buf){
	char name[512], line[256*3+1];
	int cmap, i, n;
	PICFILE *pf;
	double gamma;
	RGB p9map[256];
	cmap=-1;
	for(i=0;cmapdir[i];i++){
		sprint(name, "%s%s", cmapdir[i], f);
		if((cmap=open(name, OREAD))!=-1) break;
	}
	if(cmap==-1){ /* could be gamma or gamma<number> or fb */
		if(strcmp(f, "fb")==0){
			rdcolmap(&screen, p9map);
			for(i=0;i!=256;i++){
				buf[3*i+0]=p9map[i].red>>24;
				buf[3*i+1]=p9map[i].green>>24;
				buf[3*i+1]=p9map[i].blue>>24;
			}
			return 1;
		}
		if(strncmp(f, "gamma", 5)!=0) return 0;
		f+=5;
		if(*f=='\0') gamma=2.3;
		else{
			if(strspn(f, "0123456789.")!=strlen(f)) return 0;
			gamma=atof(f);
		}
		for(i=0;i!=256;i++){
			buf[0]=buf[1]=buf[2]=255.*pow(i/255., 1./gamma);
			buf+=3;
		}
		return 1;
	}
	n=read(cmap, line, 5);
	if(n==5 && strncmp(line, "TYPE=", 5)==0){
		close(cmap);
		pf=picopen_r(name);
		if(pf->cmap==0){
			picclose(pf);
			return 0;
		}
		memcpy(buf, pf->cmap, 3*256);
		return 1;
	}
	n+=read(cmap, line+5, 256*3-4);
	close(cmap);
	switch(n){
	default: return 0;
	case 256*3:	/* real rgb colormap */
		memcpy(buf, line, 256*3);
		break;
	case 256:	/* compensation table */
		for(i=0;i!=256;i++){
			buf[0]=buf[1]=buf[2]=line[i];
			buf+=3;
		}
		break;
	}
	return 1;
}
