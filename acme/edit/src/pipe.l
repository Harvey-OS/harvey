#include <alef.h>
#include <bio.h>

byte	*prog = "pipe";

#include "findfile.h"

void	run(byte**, int*, int*, chan(int));
void	send(byte*, int, int);

Biobuf bin;

void
main(int argc, byte **argv)
{
	int nf, afd, dfd, cfd, nc, nr, p1[2], p2[2], npart;
	int i, n, id, seq;
	byte buf[512];
	byte *s;
	byte *tmp, *data;
	Rune r;
	File *f, *tf;
	uint q, q0, q1;
	chan(int) cpid;

	if(argc < 2){
		fprint(2, "usage: pipe command\n");
		exits(nil);
	}

	#include "input.h"

	/* sort back to original order*/
	qsort(f, nf, sizeof f[0], scmp);

	/* pipe */
	id = -1;
	afd = -1;
	cfd = -1;
	dfd = -1;
	tmp = malloc(8192+UTFmax);
	if(tmp == nil)
		error("malloc");
	alloc cpid;
	for(i=0; i<nf; i++){
		tf = &f[i];
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

		if(fprint(afd, "#%ud", tf->q0) < 0)
			rerror("writing address");

		q0 = tf->q0;
		q1 = tf->q1;
		/* suck up data */
		data = malloc((q1-q0)*UTFmax+1);
		if(data == nil)
			error("malloc failed\n");
		s = data;
		q = q0;
		while(q < q1){
			nc = read(dfd, s, (q1-q)*UTFmax);
			if(nc <= 0)
				error("read error from acme");
			seek(afd, 0, 0);
			if(read(afd, buf, 12) != 12)
				rerror("reading address");
			q = atoi(buf);
			s += nc;
		}
		s = data;
		for(nr=0; nr<q1-q0; nr++)
			s += chartorune(&r, s);

		if(pipe(p1)<0 || pipe(p2)<0)
			error("pipe");

		proc run(argv+1, p1, p2, cpid);
		<-cpid;
		close(p1[0]);
		close(p2[1]);

		proc send(data, s-data, p1[1]);

		/* put back data */
		if(fprint(afd, "#%d,#%d", q0, q1) < 0)
			rerror("writing address");

		npart = 0;
		q1 = q0;
		while((nc = read(p2[0], tmp+npart, 8192)) > 0){
			nc += npart;
			s = tmp;
			while(s <= tmp+nc-UTFmax){
				s += chartorune(&r, s);
				q1++;
			}
			if(s > tmp)
				if(write(dfd, tmp, s-tmp) != s-tmp)
					error("write error to acme");
			npart = nc - (s-tmp);
			memmove(tmp, s, npart);
		}
		if(npart){
			s = tmp;
			while(s < tmp+npart){
				s += chartorune(&r, s);
				q1++;
			}
			if(write(dfd, tmp, npart) != npart)
				error("write error to acme");
		}
		if(fprint(afd, "#%d,#%d", q0, q1) < 0)
			rerror("writing address");
		if(fprint(cfd, "dot=addr\n") < 0)
			rerror("writing dot");
		free(data);
	}
}

void
run(byte **argv, int *p1, int *p2, chan(int) c)
{
	byte buf[256];

	rfork(RFFDG);
	dup(p1[0], 0);
	dup(p2[1], 1);
	close(p1[0]);
	close(p1[1]);
	close(p2[0]);
	close(p2[1]);
	c <-= getpid();
	exec(argv[0], argv);
	sprint(buf, "/bin/%s", argv[0]);
	exec(buf, argv);
	exits("can't exec");
}

void
send(byte *buf, int nbuf, int fd)
{
	if(write(fd, buf, nbuf) != nbuf)
		error("write error to process");
	close(fd);
}
