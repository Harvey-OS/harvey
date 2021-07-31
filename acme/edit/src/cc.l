#include <alef.h>
#include <bio.h>

byte	*prog = "c";

#include "findfile.h"

Biobuf bin;

void
main(int argc, byte **argv)
{
	int afd, cfd, dfd, i, j, id;
	byte buf[512];
	byte *r;
	int nf, n, seq, rlen;
	File *f, *tf;
	byte *s;

	if(argc != 2){
		fprint(2, "usage: %s 'replacement'\n", prog);
		exits(nil);
	}

	#include "input.h"

	/* sort back to original order, backwards */
	qsort(f, nf, sizeof f[0], bscmp);

	/* change */
	id = -1;
	afd = -1;
	cfd = -1;
	dfd = -1;
	rlen = strlen(argv[1]);
	r = malloc(rlen);
	if(r == nil)
		rerror("malloc");
	i = 0;
	for(j=0; j<rlen; j++){
		r[i] = argv[1][j];
		if(i>0 && r[i-1]=='\\'){
			if(r[i] == 'n')
				r[--i] = '\n';
			else if(r[i]=='\\')
				r[--i] = '\\';
		}
		i++;
	}
	rlen = i;
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
			if(write(cfd, "mark\nnomark\n", 12) != 12)
				rerror("setting nomark");
		}
		if(fprint(afd, "#%d,#%d", tf->q0, tf->q1) < 0)
			rerror("writing address");
		if(write(dfd, r, rlen) != rlen)
			rerror("writing replacement");
	}
	exits(nil);
}
