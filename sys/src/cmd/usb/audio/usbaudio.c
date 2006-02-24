/*
 * USB audio driver for Plan 9
 */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbaudio.h"
#include "usbaudioctl.h"

#define STACKSIZE 16*1024

extern char* srvpost;
char * mntpt;

Channel *controlchan;

char audstr[]		= "Enabled 0x000101";	/* audio.control.0 */

int defaultspeed[2] = {44100, 44100};

static void
audio_endpoint(Device *d, int c, ulong csp, void *bb, int n)
{
	int ifc;
	int dalt;
	byte *b = bb;

	if (c >= nelem(d->config)) {
		fprint(2, "Too many interfaces (%d of %d)\n",
			c, nelem(d->config));
		return;
	}
	dalt=csp>>24;
	ifc = csp>>16 & 0xff;

	switch(b[2]) {
	case 0x01:
		if (debug){
			fprint(2, "CS_ENDPOINT for attributes 0x%x, lockdelayunits %d, lockdelay %#ux, ",
				b[3], b[4], b[5] | (b[6]<<8));
			if (b[3] & has_setspeed)
				fprint(2, "has sampling-frequency control");
			else
				fprint(2, "does not have sampling-frequency control");
			if (b[3] & 0x1<<1)
				fprint(2, ", has pitch control");
			else
				fprint(2, ", does not have pitch control");
			if (b[3] & 0x1<<7)
				fprint(2, ", max packets only");
			fprint(2, "\n");
		}
		if (d->config[c] == nil)
			sysfatal("d->config[%d] == nil", c);
		if (d->config[c]->iface[ifc] == nil)
			sysfatal("d->config[%d]->iface[%d] == nil", c, ifc);
		if (d->config[c]->iface[ifc]->dalt[dalt] == nil)
			d->config[c]->iface[ifc]->dalt[dalt] = mallocz(sizeof(Dalt),1);
		if (d->config[c]->iface[ifc]->dalt[dalt]->devspec == nil)
			d->config[c]->iface[ifc]->dalt[dalt]->devspec= mallocz(sizeof(Audioalt),1);
		((Audioalt*)d->config[c]->iface[ifc]->dalt[dalt]->devspec)->caps |= b[3];
		break;
	case 0x02:
		if (debug){
			fprint(2, "CS_INTERFACE FORMAT_TYPE %d, channels %d, subframesize %d, resolution %d, freqtype %d, ",
				b[3], b[4], b[5], b[6], b[7]);
			fprint(2, "freq0 %d, freq1 %d\n",
				b[8] | (b[9]<<8) | (b[10]<<16), b[11] | (b[12]<<8) | (b[13]<<16));
		}
		break;
	default:
		if (debug) pcs_raw("CS_INTERFACE", bb, n);
	}
}

void (*dprinter[])(Device *, int, ulong, void *b, int n) = {
	[STRING] pstring,
	[DEVICE] pdevice,
	[0x21] phid,
	[0x24] audio_interface,
	[0x25] audio_endpoint,
};

enum {
	None,
	Volumeset,
	Volumeget,
	Altset,
	Altget,
	Speedget,
};

void
controlproc(void *)
{
	/* Proc that looks after /dev/usb/%d/ctl */
	int i, nf;
	char *req, *args[8];
	Audiocontrol *c;
	long value[8];
	Channel *replchan;

	while(req = recvp(controlchan)){
		int rec;

		nf = tokenize(req, args, nelem(args));
		if (nf < 3)
			sysfatal("controlproc: not enough arguments");
		replchan = (Channel*)strtol(args[0], nil, 0);
		if (strcmp(args[2], "playback") == 0)
			rec = Play;
		else if (strcmp(args[2], "record") == 0)
			rec = Record;
		else{
			/* illegal request */
			if (debug) fprint(2, "%s must be record or playback", args[2]);
			if (replchan) chanprint(replchan, "%s must be record or playback", args[2]);
			free(req);
			continue;
		}
		c = nil;
		for (i = 0; i < Ncontrol; i++){
			c = &controls[rec][i];
			if (strcmp(args[1], c->name) == 0)
				break;
		}
		if (i == Ncontrol){
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
				fprint(2, "controlproc: setcontrol %s %s %s\n",
					rec?"in":"out", args[1], args[3]);
			if (setcontrol(rec, args[1], value) < 0){
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
buttonproc(void *) {
	int	i, fd, b;
	char fname[64], err[32];
	byte buf[1];
	Audiocontrol *c;

	sprint(fname, "/dev/usb%d/%d/ep%ddata", ad->ctlrno, ad->id, buttonendpt);
	if (debug & Dbginfo) fprint(2, "buttonproc opening %s\n", fname);
	if ((fd = open(fname, OREAD)) < 0)
		sysfatal("Can't open %s: %r", fname);

	c = &controls[Play][Volume_control];
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
	Endpt *ep;
	int i, rec;

	for (i = 0; i < Nendpt; i++) {
		if ((ep = ad->ep[i]) == nil)
			continue;
		switch(ep->csp){
		default:
			break;
		case CSP(CL_AUDIO, 2, 0):
			if (ep->iface == nil)
				break;
			rec = (ep->addr &  0x80)?1:0;
			if (verbose)
				fprint(2, "%s on endpoint %d\n", rec?"Record":"Playback", i);
			endpt[rec] = i;
			interface[rec] = ep->iface->interface;
			break;
		case CSP(CL_HID, 0, 0):
			if (verbose)
				fprint(2, "Buttons on endpoint %d\n", i);
			buttonendpt = i;
			break;
		}
	}
}

void
usage(void)
{
	fprint(2, "usage: usbaudio [-V] [-v volume] [-m mountpoint] [-s srvname] [ctrlno n]\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char **argv)
{
	int ctlrno, id, i, sfd;
	long value[8];
	long volume[8];
	Audiocontrol *c;
	char buf[32], *p, line[256];
	extern int attachok;

	ctlrno = -1;
	id = -1;
	volume[0] = Undef;
	for (i = 0; i<8; i++)
		value[i] = 0;
	fmtinstall('A', Aconv);
	quotefmtinstall();

	ARGBEGIN{
	case 'V':
		verbose++;
		break;
	case 'd':
		debug = strtol(EARGF(usage()), nil, 0);
		if (debug == -1) debugdebug++;
		verbose++;
		break;
	case 'v':
		volume[0] = strtol(EARGF(usage()), &p, 0);
		for(i = 1; i < 8; i++)
			volume[i] = volume[0];
		break;
	case 'm':
		mntpt = EARGF(usage());
		break;
	case 's':
		srvpost = EARGF(usage());
		break;
	case 'p':
		attachok++;
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

	ad = opendev(ctlrno, id);

	if (describedevice(ad) < 0)
		sysfatal("describedevice");

	for(i=0; i<ad->nconf; i++) {
		if (ad->config[i] == nil)
			ad->config[i] = mallocz(sizeof(*ad->config[i]),1);
		loadconfig(ad, i);
	}

	controlchan = chancreate(sizeof(char*), 8);

	findendpoints();

	if (endpt[Play] >= 0){
		if(verbose)
			fprint(2, "Setting default play parameters: %d Hz, %d channels at %d bits\n",
				defaultspeed[Play], 2, 16);
		if(findalt(Play, 2, 16, defaultspeed[Play]) < 0){
			if(findalt(Play, 2, 16, 48000) < 0)
				sysfatal("Can't configure playout for %d or %d Hz", defaultspeed[Play], 48000);
			fprint(2, "Warning, can't configure playout for %d Hz, configuring for %d Hz instead\n",
				defaultspeed[Play], 48000);
			defaultspeed[Play] = 48000;
		}
		value[0] = 2;
		if (setcontrol(Play, "channels", value) == Undef)
			sysfatal("Can't set play channels\n");
		value[0] = 16;
		if (setcontrol(Play, "resolution", value) == Undef)
			sysfatal("Can't set play resolution\n");
	}

	if (endpt[Record] >= 0){
		if(verbose)
			fprint(2, "Setting default record parameters: %d Hz, %d channels at %d bits\n",
				defaultspeed[Play], 2, 16);
		if(findalt(Record, 2, 16, defaultspeed[Record]) < 0){
			if(findalt(Record, 2, 16, 48000) < 0)
				sysfatal("Can't configure record for %d or %d Hz", defaultspeed[Record], 48000);
			fprint(2, "Warning, can't configure record for %d Hz, configuring for %d Hz instead\n",
				defaultspeed[Record], 48000);
			defaultspeed[Record] = 48000;
		}
		value[0] = 2;
		if (setcontrol(Record, "channels", value) == Undef)
			sysfatal("Can't set record channels\n");
		value[0] = 16;
		if (setcontrol(Record, "resolution", value) == Undef)
			sysfatal("Can't set record resolution\n");
	}

	getcontrols();	/* Get the initial value of all controls */
	value[0] = defaultspeed[Play];
	if (endpt[Play] >= 0 && setcontrol(Play, "speed", value) < 0)
		sysfatal("Can't set play speed\n");
	value[0] = defaultspeed[Record];
	if (endpt[Record] >= 0 && setcontrol(Record, "speed", value) < 0)
		fprint(2, "Can't set record speed\n");
	value[0] = 0;
	setcontrol(Play, "mute", value);

	if (volume[0] != Undef){
		c = &controls[Play][Volume_control];
		if (*p == '%' && c->min != Undef)
			for (i = 0; i < 8; i++)
				volume[i] = (volume[i]*c->max + (100-volume[i])*c->min)/100;
		if (c->settable)
			setcontrol(Play, "volume", volume);
		c = &controls[Record][Volume_control];
		if (c->settable)
			setcontrol(Record, "volume", volume);
	}

	if (buttonendpt > 0){
		sprint(buf, "ep %d bulk r 1 1", buttonendpt);
		if (debug) fprint(2, "sending `%s' to /dev/usb/%d/ctl\n", buf, id);
		if (write(ad->ctl, buf, strlen(buf)) > 0)
			proccreate(buttonproc, nil, STACKSIZE);
		else
			fprint(2, "Could not configure button endpoint: %r\n");
	}
	proccreate(controlproc, nil, STACKSIZE);
	proccreate(serve, nil, STACKSIZE);

	threadexits(nil);
}
