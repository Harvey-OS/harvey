#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"

static int
readnum(int fd)
{
	int n;
	char buf[20];

	n = read(fd, buf, sizeof buf);
	buf[sizeof(buf)-1] = 0;
	return n <= 0? -1: strtol(buf, nil, 0);
}

Device*
opendev(int ctlrno, int id)
{
	int isnew;
	Device *d;
	char name[100], *p;

	d = emallocz(sizeof(Device), 1);
	incref(d);

	isnew = 0;
	if(id == -1) {
		sprint(name, "/dev/usb%d/new", ctlrno);
		if((d->ctl = open(name, ORDWR)) < 0){
		Error0:
			close(d->ctl);
			werrstr("open %s: %r", name);
			free(d);
			/* return nil; */
			sysfatal("%r");
		}
		id = readnum(d->ctl);
		isnew = 1;
	}
	sprint(name, "/dev/usb%d/%d/", ctlrno, id);
	p = name+strlen(name);

	if(!isnew) {
		strcpy(p, "ctl");
		if((d->ctl = open(name, ORDWR)) < 0)
			goto Error0;
	}

	strcpy(p, "setup");
	if((d->setup = open(name, ORDWR)) < 0){
	Error1:
		close(d->setup);
		goto Error0;
	}

	strcpy(p, "status");
	if((d->status = open(name, OREAD)) < 0)
		goto Error1;

	d->ctlrno = ctlrno;
	d->id = id;
	return d;
}

void
closedev(Device *d)
{
	int i, j;
	Dconf *conf;
	Dalt *alt, *nextalt;

	if(d==nil)
		return;
	if(decref(d) != 0)
		return;
	close(d->ctl);
	close(d->setup);
	close(d->status);

	for(i = 0; i < d->nconf; i++) {
		conf = &d->config[i];
		for(j = 0; j < conf->nif; j++) {
			for(alt = conf->iface[j].alt; alt != nil; alt = nextalt) {
				nextalt = alt->next;
				free(alt->ep);
				free(alt);
			}
		}
		free(conf->iface);
	}
	free(d->config);
	free(d);
}
