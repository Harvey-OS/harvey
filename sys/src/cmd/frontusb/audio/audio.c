#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "usb.h"

typedef struct Range Range;
struct Range
{
	Range	*next;
	int	min;
	int	max;
};

typedef struct Aconf Aconf;
struct Aconf
{
	Range	*freq;
	int	caps;
};

int audiodelay = 1764;	/* 40 ms */
int audiofreq = 44100;
int audiochan = 2;
int audiores = 16;

char user[] = "audio";

Dev *audiodev = nil;

Ep *audioepin = nil;
Ep *audioepout = nil;

void
parsedescr(Desc *dd)
{
	Aconf *c;
	Range *f;
	uchar *b;
	int i;

	if(dd == nil || dd->iface == nil || dd->altc == nil)
		return;
	if(Subclass(dd->iface->csp) != 2)
		return;

	c = dd->altc->aux;
	if(c == nil){
		c = mallocz(sizeof(*c), 1);
		dd->altc->aux = c;
	}

	b = (uchar*)&dd->data;
	switch(b[1]<<8 | b[2]){
	case 0x2501:	/* CS_ENDPOINT, EP_GENERAL */
		c->caps |= b[3];
		break;

	case 0x2402:	/* CS_INTERFACE, FORMAT_TYPE */
		if(b[4] != audiochan)
			break;
		if(b[6] != audiores)
			break;

		if(b[7] == 0){
			f = mallocz(sizeof(*f), 1);
			f->min = b[8] | b[9]<<8 | b[10]<<16;
			f->max = b[11] | b[12]<<8 | b[13]<<16;

			f->next = c->freq;
			c->freq = f;
		} else {
			for(i=0; i<b[7]; i++){
				f = mallocz(sizeof(*f), 1);
				f->min = b[8+3*i] | b[9+3*i]<<8 | b[10+3*i]<<16;
				f->max = f->min;

				f->next = c->freq;
				c->freq = f;
			}
		}
		break;
	}
}

Dev*
setupep(Dev *d, Ep *e, int speed)
{
	Altc *x;
	Aconf *c;
	Range *f;
	int i;

	for(i = 0; i < nelem(e->iface->altc); i++){
		if((x = e->iface->altc[i]) == nil)
			continue;
		if((c = x->aux) == nil)
			continue;
		for(f = c->freq; f != nil; f = f->next)
			if(speed >= f->min && speed <= f->max)
				goto Foundaltc;
	}
	werrstr("no altc found");
	return nil;

Foundaltc:
	if(usbcmd(d, Rh2d|Rstd|Riface, Rsetiface, i, e->iface->id, nil, 0) < 0){
		werrstr("set altc: %r");
		return nil;
	}

	if(c->caps & 1){
		uchar b[4];

		b[0] = speed;
		b[1] = speed >> 8;
		b[2] = speed >> 16;
		if(usbcmd(d, Rh2d|Rclass|Rep, Rsetcur, 0x100, e->addr, b, 3) < 0)
			fprint(2, "warning: set freq: %r\n");
	}

	if((d = openep(d, e->id)) == nil){
		werrstr("openep: %r");
		return nil;
	}
	devctl(d, "pollival %d", x->interval);
	devctl(d, "samplesz %d", audiochan*audiores/8);
	devctl(d, "sampledelay %d", audiodelay);
	devctl(d, "hz %d", speed);
	if(e->dir == Ein)
		devctl(d, "name audioin");
	else
		devctl(d, "name audio");
	return d;
}

void
fsread(Req *r)
{
	char *msg;

	msg = smprint("master 100 100\nspeed %d\ndelay %d\n", audiofreq, audiodelay);
	readstr(r, msg);
	respond(r, nil);
	free(msg);
}

void
fswrite(Req *r)
{
	char msg[256], *f[4];
	int nf, speed;

	snprint(msg, sizeof(msg), "%.*s",
		utfnlen((char*)r->ifcall.data, r->ifcall.count), (char*)r->ifcall.data);
	nf = tokenize(msg, f, nelem(f));
	if(nf < 2){
		respond(r, "invalid ctl message");
		return;
	}
	if(strcmp(f[0], "speed") == 0){
		Dev *d;

		speed = atoi(f[1]);
Setup:
		if((d = setupep(audiodev, audioepout, speed)) == nil){
			responderror(r);
			return;
		}
		closedev(d);
		if(audioepin != nil && audioepin != audioepout){
			if(d = setupep(audiodev, audioepin, speed))
				closedev(d);
		}
		audiofreq = speed;
	} else if(strcmp(f[0], "delay") == 0){
		audiodelay = atoi(f[1]);
		speed = audiofreq;
		goto Setup;
	}
	r->ofcall.count = r->ifcall.count;
	respond(r, nil);
}

Srv fs = {
	.read = fsread,
	.write = fswrite,
};

void
usage(void)
{
	fprint(2, "%s devid\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	char buf[32];
	Dev *d, *ed;
	Ep *e;
	int i;

	ARGBEGIN {
	case 'D':
		chatty9p++;
		break;
	case 'd':
		usbdebug++;
		break;
	} ARGEND;

	if(argc == 0)
		usage();

	if((d = getdev(*argv)) == nil)
		sysfatal("getdev: %r");
	audiodev = d;

	/* parse descriptors, mark valid altc */
	for(i = 0; i < nelem(d->usb->ddesc); i++)
		parsedescr(d->usb->ddesc[i]);
	for(i = 0; i < nelem(d->usb->ep); i++){
		e = d->usb->ep[i];
		if(e != nil && e->type == Eiso && e->iface != nil && e->iface->csp == CSP(Claudio, 2, 0)){
			switch(e->dir){
			case Ein:
				if(audioepin != nil)
					continue;
				audioepin = e;
				break;
			case Eout:
				if(audioepout != nil)
					continue;
				audioepout = e;
				break;
			case Eboth:
				if(audioepin != nil && audioepout != nil)
					continue;
				if(audioepin == nil)
					audioepin = e;
				if(audioepout == nil)
					audioepout = e;
				break;
			}
			if((ed = setupep(audiodev, e, audiofreq)) == nil){
				fprint(2, "setupep: %r\n");

				if(e == audioepin)
					audioepin = nil;
				if(e == audioepout)
					audioepout = nil;
				continue;
			}
			closedev(ed);
		}
	}
	if(audioepout == nil)
		sysfatal("no endpoints found");

	fs.tree = alloctree(user, "usb", DMDIR|0555, nil);
	createfile(fs.tree->root, "volume", user, 0666, nil);

	snprint(buf, sizeof buf, "%d.audio", audiodev->id);
	postsharesrv(&fs, nil, "usb", buf);

	exits(0);
}
