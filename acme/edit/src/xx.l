#include <alef.h>
#include <bio.h>

byte	*prog = "x";

#include "findfile.h"

Biobuf bin;

void
main(int argc, byte **argv)
{
	int afd, cfd, dfd, i, id, seq;
	byte buf[512];
	int nf, n, plen;
	int addr, aq0, aq1, matched;
	File *f, *tf;
	byte *s;

	if(argc!=2 || strlen(argv[1])==0 || argv[1][0]!='/'){
		fprint(2, "usage: %s '/regexp/'\n", prog);
		exits(nil);
	}

	#include "input.h"

	/* execute regexp */
	id = -1;
	afd = -1;
	dfd = -1;
	cfd = -1;
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
			sprint(buf, "/mnt/acme/%d/ctl", id);
			cfd = open(buf, ORDWR);
			if(cfd < 0)
				rerror(buf);
			sprint(buf, "/mnt/acme/%d/data", id);
			dfd = open(buf, ORDWR);
			if(dfd < 0)
				rerror(buf);
		}
		n = strlen(tf->addr);
		if(write(afd, tf->addr, n)!=n || fprint(cfd, "limit=addr\n")<0){
			sprint(buf, "%s:%s is invalid limit", tf->name, tf->addr);
			rerror(buf);
		}
		if(fprint(afd, "#%d", tf->q0) < 0)
			rerror("can't set address");
		if(fprint(cfd, "dot=addr") < 0)
			rerror("can't unset dot");
		addr = tf->q0-1;
		plen = strlen(argv[1]);
		matched = 0;
		/* scan for matches */
		for(;;){
			if(write(afd, argv[1], plen) != plen)
				break;
			seek(afd, 0, 0);
			if(read(afd, buf, sizeof buf) != 2*12)
				rerror("reading address");
			aq0 = atoi(buf);
			aq1 = atoi(buf+12);
			if(matched && aq1==aq0 && addr==aq1){	/* repeated null match; advance */
				matched = 0;
				addr++;
				if(addr > tf->q1)
					break;
				if(fprint(afd, "#%d", addr) < 0)
					rerror("writing address");
				continue;
			}
			matched = 1;
			if(aq0<addr || aq0>=tf->q1 || aq1>tf->q1)
				break;
			addr = aq1;
			if(aq0 == aq1)
				print("%s:#%d\n", tf->name, aq0);
			else
				print("%s:#%d,#%d\n", tf->name, aq0, aq1);
		}
	}
	exits(nil);
}
