#include <alef.h>
#include <bio.h>

byte	*prog = "p";

#include "findfile.h"

Biobuf bin;

void
main(int argc, byte **argv)
{
	int afd, cfd, dfd, i, id;
	byte buf[512];
	int m, nr, nf, n, nflag, seq;
	File *f, *tf;
	byte *s;

	nflag = 0;
	if(argc==2 && strcmp(argv[1], "-n")==0){
		--argc;
		nflag = 1;
	}
	if(argc != 1){
		fprint(2, "usage: %s [-n]\n", prog);
		exits(nil);
	}
	
	#include "input.h"

	/* sort back to original order*/
	qsort(f, nf, sizeof f[0], scmp);

	/* print */
	id = -1;
	afd = -1;
	cfd = -1;
	dfd = -1;
	for(i=0; i<nf; i++){
		tf = &f[i];
		if(tf->ok == 0)
			continue;
		if(tf->id != id){
			if(id > 0){
				close(afd);
				close(cfd);
				close(dfd);
			}
			id = tf->id;
			sprint(buf, "/mnt/acme/%d/addr", id);
			afd = open(buf, ORDWR);
			if(afd < 0)
				rerror(buf);
			sprint(buf, "/mnt/acme/%d/data", id);
			dfd = open(buf, ORDWR);
			if(dfd < 0)
				rerror(buf);
			sprint(buf, "/mnt/acme/%d/ctl", id);
			cfd = open(buf, ORDWR);
			if(cfd < 0)
				rerror(buf);
		}
		if(nflag){
			if(tf->q1 > tf->q0)
				print("%s:#%d,#%d: ", tf->name, tf->q0, tf->q1);
			else
				print("%s:#%d: ", tf->name, tf->q0);
		}
		m = tf->q0;
		while(m < tf->q1){
			if(fprint(afd, "#%d", m) < 0){
				fprint(2, "%s: %s:%s is invalid address\n", prog, tf->name, tf->addr);
				continue;
			}
			n = read(dfd, buf, sizeof buf);
			nr = nrunes(buf, n);
			while(m+nr > tf->q1){
				do; while(n>0 && (buf[--n]&0xC0)==0x80);
				--nr;
			}
			if(n == 0)
				break;
			write(1, buf, n);
			m += nr;
		}
	}
	exits(nil);
}
