#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbproto.h"
#include "dat.h"
#include "fns.h"
#include "audioclass.h"

#define STACKSIZE 16*1024

extern int srvpost;
char * mntpt;
int debug;
int debugdebug;
int verbose;

Channel *controlchan;

char audstr[]		= "Enabled 0x000101\n";	/* audio.control.0 */

Nexus *nexushd;
Nexus *nexustl;

void
makenexus(Audiofunc *af, Funcalt *alt, Unit *output)
{
	int i, ch;
	Nexus *nx;
	Unit *u, *feat;
	static int idgen;
	Audiocontrol *c;
	Stream *ins, *outs;

	nx = emallocz(sizeof(Nexus), 1);
	nx->af = af;
	nx->alt = alt;
	nx->output = output;
	nx->id = idgen++;
	for(u = output; u != nil; u = u->source[0]) {
		switch(u->type) {
		case INPUT_TERMINAL:
			nx->input = u;
			break;
		case FEATURE_UNIT:
			nx->feat = u;
			break;
		}
		if(u->nsource == 0)
			break;
	}
	if(nx->input == nil) {
		free(nx);
		return;
	}
	ins = nx->output->stream;
	outs = nx->input->stream;
	if(ins == nil && outs == nil || ins != nil && outs != nil) {
		free(nx);
		return;
	}
	if(ins != nil) {
		nx->s = ins;
		nx->dir = Nin;
	}
	else {
		nx->s = outs;
		nx->dir = Nout;
	}

	memset(nx->control, 0, sizeof(nx->control));
	for(i = 0; i < Ncontrol; i++) {
		c = &nx->control[i];
		c->step = Undef;
	}
	calcbounds(nx);
	feat = nx->feat;
	if(feat != nil) {
		for(i = Mute_control; i <= Loudness_control; i++) {
			c = &nx->control[i];
			for(ch = 0; ch <= feat->nchan; ch++) {
				if((feat->hascontrol[ch] & (1<<(i-1))) == 0)
					continue;
				c->readable = 1;
				c->settable = 1;
				if(ch > 0)
					c->chans |= (1<<ch);
			}
		}
	}

	if(nexushd == nil)
		nexushd = nx;
	else
		nexustl->next = nx;
	nexustl = nx;
}

int
adddevice(Device *d)
{
	int i;
	Unit *u;
	Dconf *c;
	Dinf *intf;
	ulong csp;
	Audiofunc *af;

	if(d->nconf < 1)
		sysfatal("no configurations???");

	/* we only handle first config for now -- most devices only have 1 anyway */
	c = &d->config[0];
	for(i = 0; i < c->nif; i++) {
		intf = &c->iface[i];
		if(intf == nil)
			continue;
		csp = intf->csp;
		if(Class(csp) == CL_AUDIO && Subclass(csp) == AUDIOCONTROL) {
			if((af = getaudiofunc(intf)) == nil)
				return -1;
			if(debug)
				dumpaudiofunc(af);
			for(u = af->falt->unit; u != nil; u = u->next)
				if(u->type == OUTPUT_TERMINAL)
					makenexus(af, af->falt, u);
		}
		if(Class(csp) == CL_HID && Subclass(csp) == 0) {
			if (verbose)
				fprint(2, "Buttons on endpoint %d\n", i);
			if(intf->alt->npt == 1)
				buttonendpt = intf->alt->ep[0].addr;
		}
	}
	return 0;
}

void
controlproc(void *)
{
	/* Proc that looks after /dev/usb/%d/ctl */
	int nf;
	Nexus *nx;
	long value[8];
	Audiocontrol *c;
	char *req, *args[8];
	Channel *replchan;

	while(req = recvp(controlchan)) {
		nf = tokenize(req, args, nelem(args));
		if (nf < 3)
			sysfatal("controlproc: not enough arguments");
		replchan = (Channel*)strtol(args[0], nil, 0);
		nx = findnexus(args[2]);
		if(nx == nil) {
			/* illegal request */
			if (debug) fprint(2, "%s must be record or playback", args[2]);
			if (replchan) chanprint(replchan, "%s must be record or playback", args[2]);
			free(req);
			continue;
		}
		c = findcontrol(nx, args[1]);
		if (c == nil){
			if (debug) fprint(2, "Illegal control name: %s", args[1]);
			if (replchan) chanprint(replchan, "Illegal control name: %s", args[1]);
		}else if (!c->settable){
			if (debug & Dbginfo) fprint(2, "%s %s is not settable", args[1], args[2]);
			if (replchan)
				chanprint(replchan, "%s %s is not settable", args[1], args[2]);
		}else if (nf < 4){
			if (debug & Dbginfo) fprint(2, "insufficient arguments for %s %s",
					args[1], args[2]);
			if (replchan)
				chanprint(replchan, "insufficient arguments for %s %s",
					args[1], args[2]);
		}else if (ctlparse(args[3], c, value) < 0) {
			if (replchan)
				chanprint(replchan, "parse error in %s %s", args[1], args[2]);
		} else {
			if (debug & Dbginfo)
				fprint(2, "controlproc: setcontrol %s %s %s\n", nx->name, args[1], args[3]);
			if (setcontrol(nx, args[1], value) < 0){
				if (replchan)
					chanprint(replchan, "setting %s %s failed", args[1], args[2]);
			}else{
				if (replchan) chanprint(replchan, "ok");
			}
			ctlevent();
		}
		free(req);
	}
}

void
buttonproc(void *x)
{
	int i, fd, b;
	Device *d;
	uchar buf[1];
	Audiocontrol *c;
	char fname[64], err[32];

	d = x;
	sprint(fname, "/dev/usb%d/%d/ep%ddata", d->ctlrno, d->id, buttonendpt);
	if (debug & Dbginfo) fprint(2, "buttonproc opening %s\n", fname);
	if ((fd = open(fname, OREAD)) < 0)
		sysfatal("Can't open %s: %r", fname);

	c = &nexus[Play]->control[Volume_control];
	for (;;) {
		if ((b = read(fd, buf, 1)) < 0){
			rerrstr(err, sizeof err);
			if (strcmp(err, "interrupted") == 0){
				if (debug & Dbginfo) fprint(2, "read interrupted\n");
				continue;
			}
			sysfatal("read %s: %r", fname);
		}
		if (b == 0 || buf[0] == 0){
			continue;
		}else if (buf[0] == 1){
			if (c->chans == 0)
				c->value[0] += c->step;
			else
				for (i = 1; i < 8; i++)
					if (c->chans & 1 << i)
						c->value[i] += c->step;
			chanprint(controlchan, "0 volume playback %A", c);
		}else if (buf[0] == 2){
			if (c->chans == 0)
				c->value[0] -= c->step;
			else
				for (i = 1; i < 8; i++)
					if (c->chans & 1 << i)
						c->value[i] -= c->step;
			chanprint(controlchan, "0 volume playback %A", c);
		}else if (debug & Dbginfo){
			fprint(2, "button");
			for (i = 0; i < b; i++)
				fprint(2, " %#2.2x", buf[i]);
			fprint(2, "\n");
		}
	}
}

void
findendpoints(void)
{
	Nexus *nx;

	for(nx = nexushd; nx != nil; nx = nx->next) {
		switch(nx->dir) {
		case Nin:
			nx->name = "record";
			nexus[Record] = nx;
			break;
		case Nout:
			nx->name = "play";
			nexus[Play] = nx;
			break;
		}
	}
}

void
usage(void)
{
	fprint(2, "usage: usbaudio [-d] [-v volume[%%]] [-m mountpoint] [ctrlno id]\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char **argv)
{
	Device *d;
	Nexus *nx;
	long value[8];
	long volume[8];
	Audiocontrol *c;
	int ctlrno, id, i, sfd;
	char buf[32], *p, line[256];

	ctlrno = -1;
	id = -1;
	volume[0] = Undef;
	for (i = 0; i<8; i++)
		value[i] = 0;
	fmtinstall('A', Aconv);
	quotefmtinstall();
	usbfmtinit();

	ARGBEGIN{
	case 'd':
		debug = strtol(ARGF(), nil, 0);
		if (debug == -1) debugdebug++;
		verbose++;
		break;
	case 'v':
		volume[0] = strtol(ARGF(), &p, 0);
		for(i = 1; i < 8; i++)
			volume[i] = volume[0];
		break;
	case 'm':
		mntpt = ARGF();
		break;
	case 's':
		srvpost = 1;
		break;
	default:
		usage();
	}ARGEND

	switch (argc) {
	case 0:
		for (ctlrno = 0; ctlrno < 16; ctlrno++) {
			for (i = 1; i < 128; i++) {
				sprint(buf, "/dev/usb%d/%d/status", ctlrno, i);
				sfd = open(buf, OREAD);
				if (sfd < 0)
					break;
				if (read(sfd, line, strlen(audstr)) == strlen(audstr)
				 && strncmp(audstr, line, strlen(audstr)) == 0) {
					id = i;
					goto found;
				}
				close(sfd);
			}
		}
		if (verbose) fprint(2, "No usb audio\n");
		threadexitsall("usbaudio not found");
	found:
		break;
	case 2:
		ctlrno = atoi(argv[0]);
		id = atoi(argv[1]);
		break;
	default:
		usage();
	}

	d = opendev(ctlrno, id);

	if(describedevice(d) < 0)
		sysfatal("describedevice");

	if(adddevice(d) < 0)
		sysfatal("usbaudio: adddevice: %r");

	controlchan = chancreate(sizeof(char*), 8);

	findendpoints();

	nx = nexus[Play];
	if(nx != nil) {
		value[0] = 2;
		if(setcontrol(nx, "channels", value) == Undef)
			sysfatal("Can't set play channels");
		value[0] = 16;
		if(setcontrol(nx, "resolution", value) == Undef)
			sysfatal("Can't set play resolution");
		getcontrols(nx);	/* Get the initial value of all controls */
		value[0] = 44100;
		if(setcontrol(nx, "speed", value) < 0)
			fprint(2, "warning: Can't set play speed\n");
		value[0] = 0;
		setcontrol(nx, "mute", value);
		if(volume[0] != Undef) {
			c = &nx->control[Volume_control];
			if(*p == '%' && c->min != Undef)
				for (i = 0; i < 8; i++)
					volume[i] = (volume[i]*c->max + (100-volume[i])*c->min)/100;
			if(c->settable)
				setcontrol(nx, "volume", volume);
		}
	}

	nx = nexus[Record];
	if(nx != nil) {
		value[0] = 2;
		if(setcontrol(nx, "channels", value) == Undef)
			sysfatal("Can't set record channels");
		value[0] = 16;
		if(setcontrol(nx, "resolution", value) == Undef)
			sysfatal("Can't set record resolution");
		getcontrols(nx);	/* Get the initial value of all controls */
		value[0] = 44100;
		if(setcontrol(nx, "speed", value) < 0)
			sysfatal("Can't set record speed");
		if(volume[0] != Undef) {
			c = &nx->control[Volume_control];
			if(c->settable)
				setcontrol(nx, "volume", volume);
		}
	}

	nx = nexus[Play];
	if(buttonendpt > 0 && nx != nil) {
		sprint(buf, "ep %d bulk r 1 1", buttonendpt);
		if(debug) fprint(2, "sending `%s' to /dev/usb/%d/ctl\n", buf, id);
		if(write(d->ctl, buf, strlen(buf)) > 0)
			proccreate(buttonproc, nx->s->intf->d, STACKSIZE);
		else
			fprint(2, "Could not configure button endpoint: %r\n");
	}
	proccreate(controlproc, nil, STACKSIZE);
	proccreate(serve, nil, STACKSIZE);

	threadexits(nil);
}
