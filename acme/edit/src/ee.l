#include <alef.h>
#include <bio.h>

byte	*prog = "e";

#include "findfile.h"

(int, int)	lineno(File*);

int
fcmp(File *a, File *b)
{
	int x;

	x = strcmp(a->name, b->name);
	if(x != 0)
		return x;
	x = a->q0 - b->q0;
	if(x != 0)
		return x;
	return a->q1-b->q1;
}

void
main(int argc, byte **argv)
{
	int afd, cfd, i, id;
	byte buf[512];
	int nf, n, lines, l0, l1;
	File *f, *tf;

	lines = 0;
	if(argc>1 && strcmp(argv[1], "-l")==0){
		lines = 1;
		--argc;
		argv++;
	}
	if(argc < 2){
		fprint(2, "usage: %s 'file[:address]' ...\n", prog);
		exits(nil);
	}
	nf = 0;
	f = nil;
	for(i=1; i<argc; i++){
		(n, tf) = findfile(argv[i]);
		if(n == 0)
			errors("no files match pattern", argv[i]);
		f = realloc(f, (n+nf)*sizeof(*f));
		if(f == nil)
			rerror("out of memory");
		memmove(f+nf, tf, n*sizeof(f[0]));
		nf += n;
		free(tf);
	}

	/* convert to character positions */
	for(i=0; i<nf; i++){
		tf = &f[i];
		id = tf->id;
		sprint(buf, "/mnt/acme/%d/addr", id);
		afd = open(buf, ORDWR);
		if(afd < 0)
			rerror(buf);
		sprint(buf, "/mnt/acme/%d/ctl", id);
		cfd = open(buf, ORDWR);
		if(cfd < 0)
			rerror(buf);
		if(write(cfd, "addr=dot\n", 9) != 9)
			rerror("setting address to dot");
		if(write(afd, tf->addr, strlen(tf->addr)) != strlen(tf->addr)){
			fprint(2, "%s: %s:%s is invalid address\n", prog, tf->name, tf->addr);
			tf->ok = 0;
			close(afd);
			close(cfd);
			continue;
		}
		seek(afd, 0, 0);
		if(read(afd, buf, sizeof buf) != 2*12)
			rerror("reading address");
		close(afd);
		close(cfd);
		tf->q0 = atoi(buf);
		tf->q1 = atoi(buf+12);
		tf->ok = 1;
	}

	/* sort */
	qsort(f, nf, sizeof f[0], fcmp);

	/* print */
	for(i=0; i<nf; i++){
		tf = &f[i];
		if(tf->ok)
			if(lines){
				(l0, l1) = lineno(tf);
				if(l1 > l0)
					print("%s:%d,%d\n", tf->name, l0, l1);
				else
					print("%s:%d\n", tf->name, l0);
			}else{
				if(tf->q1 > tf->q0)
					print("%s:#%d,#%d\n", tf->name, tf->q0, tf->q1);
				else
					print("%s:#%d\n", tf->name, tf->q0);
			}
	}
	exits(nil);
}

(int, int)
lineno(File *f)
{
	Biobuf *b;
	int n0, n1, q, r;
	byte buf[256];

	sprint(buf, "/mnt/acme/%d/body", f->id);
	b = Bopen(buf, OREAD);
	if(b == nil){
		fprint(2, "%s: can't open %s: %r\n", prog, buf);
		exits("open");
	}
	n0 = 1;
	n1 = 1;
	for(q=0; q<f->q1; q++){
		r = b->getrune();
		if(r == Beof){
			fprint(2, "%s: early EOF on %s\n", prog, buf);
			exits("read");
		}
		if(r=='\n'){
			if(q < f->q0)
				n0++;
			if(q+1 < f->q1)
				n1++;
		}
	}
	b->term();
	return (n0, n1);
}
