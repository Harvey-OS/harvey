#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
void filter(uchar *, uchar *, uchar *, int, int);
uchar inline[3][8192*8];
uchar outline[8192*8];
input(PICFILE *in, uchar *l, uchar *m){
	int i;
	if(!picread(in, (char *)l)){
		memcpy((char *)l-in->nchan, (char *)m-in->nchan,
			PIC_NCHAN(in)*(PIC_WIDTH(in)+2));
		return;
	}
	m=l+PIC_NCHAN(in)*PIC_WIDTH(in);
	for(i=0;i!=PIC_NCHAN(in);i++,l++,m++){
		l[-PIC_NCHAN(in)]=l[0];
		m[0]=m[-PIC_NCHAN(in)];
	}
}
main(int argc, char *argv[]){
	int y;
	uchar *l0, *l1, *l2, *t;
	PICFILE *in, *out;
	argc=getflags(argc, argv, "");
	if(argc!=1 && argc!=2) usage("[picture]");
	in=picopen_r(argc==2?argv[1]:"IN");
	if(in==0){
		perror(argc==2?argv[1]:"IN");
		exits("open input");
	}
	out=picopen_w("OUT", PIC_SAMEARGS(in));
	if(out==0){
		perror("OUT");
		exits("create output");
	}
	l0=inline[0]+in->nchan;
	l1=inline[1]+in->nchan;
	l2=inline[2]+in->nchan;
	input(in, l0, l0);
	memcpy((char *)l1, (char *)l0, PIC_NCHAN(in)*(PIC_WIDTH(in)+2));
	for(y=0;y!=PIC_HEIGHT(in);y++){
		input(in, l2, l1);
		filter(l0, l1, l2, PIC_NCHAN(in), PIC_WIDTH(in));
		picwrite(out, (char *)outline);
		t=l0;
		l0=l1;
		l1=l2;
		l2=t;
	}
	exits(0);
}
