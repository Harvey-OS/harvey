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
extern MasterVol masterplayvol;	/* K.Okamoto */
extern int mixerid;			/* K.Okamoto */
extern int selector;		/* K.Okamoto */
int vendor;		/* K.Okamoto */
int product;		/* K.Okamoto */
Channel *controlchan;

char audstr[]		= "Enabled 0x000101\n";	/* audio.control.0 */

int defaultspeed[2] = {44100, 44100};

extern FeatureAttr units[8][16];		/* K.Okamoto */
extern int nunits[8];

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
	long value[9];
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
			if (rec == Play) {		/* most messy part, K.Okamoto */
				if (vendor == 0xd8c && product== 0x6) {	/* for C-Media CM-106/F chip */
					if (setcontrol(rec, args[1], value, 0) < 0){	/* units[masterPlayVol][0]: #13 Unit */
						if (replchan)
							chanprint(replchan, "setting %s %s failed", args[1], args[2]);
					}else{
						if (replchan) chanprint(replchan, "ok");
					}
				} else {
					if (setcontrol(rec, args[1], value, 0) < 0){	/* replace the '0' to another */
						if (replchan)
							chanprint(replchan, "setting %s %s failed", args[1], args[2]);
					}else{
						if (replchan) chanprint(replchan, "ok");
					}
				}
			} else
			if (rec == Record && !strcmp(args[1], "volume")) {
				if (vendor == 0xd8c && product== 0x6) {	/* for C-Media CM-106/F chip */
					if (setcontrol(rec, args[1], value, 2) < 0){	/* units[LRRecVol][2]: #2 Unit */
						if (replchan)
							chanprint(replchan, "setting %s %s failed", args[1], args[2]);
					}else{
						if (replchan) chanprint(replchan, "ok");
					}
				}
				ctlevent();
			}
		}
		free(req);
	}
}

void
buttonproc(void *) {
	int	i, fd, b;
	char fname[64], err[32];
	byte buf[1];
	Audiocontrol *c, *cm;

	sprint(fname, "/dev/usb%d/%d/ep%ddata", ad->ctlrno, ad->id, buttonendpt);
	if (debug & Dbginfo) fprint(2, "buttonproc opening %s\n", fname);
	if ((fd = open(fname, OREAD)) < 0)
		sysfatal("Can't open %s: %r", fname);

	c = &controls[Play][Volume_control];
	cm = &controls[Play][Mute_control];
	if (vendor == 0xd8c && product== 0x6)		/* for C-Media CM-106/F chip */
		cm->value[0] = units[masterPlayMute][0].value[0];	/* K.Okamoto, messy! */
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
		}else if (buf[0] == 1){		/* increase volume */
			c->chans = 0;
			c->value[0] += c->step;
			chanprint(controlchan, "0 volume playback %A", c);
		}else if (buf[0] == 2){		/* decrease volume */
			c->chans = 0;
			c->value[0] -= c->step;
			chanprint(controlchan, "0 volume playback %A", c);
		}else if (buf[0] == 4){		/* Play mute On/Off control K.Okamoto */
			cm->chans = 0;
			if (cm->value[0] == 0)
				cm->value[0] = 1;
			else
				cm->value[0] = 0;
			chanprint(controlchan, "0 mute playback %A", cm);
		}else if (buf[0] == 0x70){		/* Onkyo SE-U55X IrDA */
			// not implemented yet
			continue;
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
	fprint(2, "usage: usbaudio [-V] [-v volume[%%]] [-m mountpoint] [-r recording device] [-s srvname] [ctrlno id]\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char **argv)
{
	int ctlrno, id, i, j, sfd, select;
	long value[9];
	long volume[9];
	long mastervol;		/* K.Okamoto */
	char buf[32], *p, line[256];

	ctlrno = -1;
	id = -1;
	volume[0] = 50;		/* default volume is 50% */
	select = 4;				/* Mixer input default, must be checked for other devices K.Okamoto */
	for (i = 0; i<9; i++)		/* K.Okamoto */
		value[i] = 0;
	fmtinstall('A', Aconv);
	quotefmtinstall();

	mastervol = 0;		/* K.Okamoto */
	ARGBEGIN{
	case 'V':
		verbose++;
		break;
	case 'd':
		debug = strtol(ARGF(), nil, 0);
		if (debug == -1) debugdebug++;
		verbose++;
		break;
	case 'v':
		mastervol = strtol(ARGF(), &p, 0);;		/* K.Okamoto */
		for(i = 0; i < 9; i++)
			volume[i] = mastervol;			/* K.Okamoto */
		break;
	case 'm':
		mntpt = ARGF();
		break;
	case 'r':
		select = atoi(ARGF());
		break;
	case 's':
		srvpost = ARGF();
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

	vendor = ad->vid;
	product = ad->did;

	for(i=0; i<ad->nconf; i++) {
		if (ad->config[i] == nil)
			ad->config[i] = mallocz(sizeof(*ad->config[i]),1);
		loadconfig(ad, i);
	}

	controlchan = chancreate(sizeof(char*), 8);

	findendpoints();

	if (endpt[Play] >= 0){
		if (vendor == 0xd8c && product == 0x6) {		/* C-Media CM-106F chip K.Okamoto */
			if(findalt(Play, 8, 16, defaultspeed[Play]) < 0){
				if(findalt(Play, 8, 16, 48000) < 0)
					sysfatal("Can't configure playout for %d or %d Hz", defaultspeed[Play], 48000);
				fprint(2, "Warning, can't configure playout for %d Hz, configuring for %d Hz instead\n",
					defaultspeed[Play], 48000);
				defaultspeed[Play] = 48000;
			}
		} else {
			if(findalt(Play, 2, 16, defaultspeed[Play]) < 0){
				if(findalt(Play, 2, 16, 48000) < 0)
					sysfatal("Can't configure playout for %d or %d Hz", defaultspeed[Play], 48000);
				fprint(2, "Warning, can't configure playout for %d Hz, configuring for %d Hz instead\n",
					defaultspeed[Play], 48000);
				defaultspeed[Play] = 48000;
			}
		}
		/* K.Okamoto choose feature 4 (connected from Mixer) for Output to USB Stream */
		if (selector >0)
			if( setselector(select) == Undef)
				sysfatal("Failed to set selector\n");
		value[0] = 8;			/* K.Okamoto */
		if (setcontrol(Play, "channels", value, featureid[Play]) == Undef)
			sysfatal("Can't set play channels\n");
		value[0] = 16;
		if (setcontrol(Play, "resolution", value, featureid[Play]) == Undef)
			sysfatal("Can't set play resolution\n");
	}

	if (endpt[Record] >= 0){
		if(findalt(Record, 2, 16, defaultspeed[Record]) < 0){
			if(findalt(Record, 2, 16, 48000) < 0)
				sysfatal("Can't configure record for %d or %d Hz", defaultspeed[Record], 48000);
			fprint(2, "Warning, can't configure record for %d Hz, configuring for %d Hz instead\n",
				defaultspeed[Record], 48000);
			defaultspeed[Record] = 48000;
		}
		/* K.Okamoto choose all the controls for Mixer inputs */
		if (mixerid >0)
			if (initmixer() == Undef)		/* K.Okamoto, set all the channels alive on the mixer */
				sysfatal("Failed to set MIXER all live\n");
		value[0] = 2;
		if (setcontrol(Record, "channels", value, featureid[Record]) == Undef)
			sysfatal("Can't set record channels\n");
		value[0] = 16;
		if (setcontrol(Record, "resolution", value, featureid[Record]) == Undef)
			sysfatal("Can't set record resolution\n");
	}

	if(debugdebug) fprint(2, "\n=======Start of listing default values of feature units=======\n");	/*.K.Okamoto */
	getcontrols();
	if(debugdebug) fprint(2, "\n=======Start of setting values of feature units=======\n");	/*.K.Okamoto */
	value[0] = defaultspeed[Play];
	if (endpt[Play] >= 0 && setcontrol(Play, "speed", value, featureid[Play]) < 0)
		sysfatal("Can't set play speed\n");
	value[0] = defaultspeed[Record];
	if (endpt[Record] >= 0 && setcontrol(Record, "speed", value, featureid[Record]) < 0)
		sysfatal("Can't set record speed\n");
	/* here, 'i' is the n-th turn of multiple Units for this purpose, 
		'i' does not equals to Unit number itself */
	if (nunits[masterRecMute])
		for(i=0; i<nunits[masterRecMute]; i++) {
			value[0] = 0;			/* set master Record Mute OFF K.Okamoto */
			setcontrol(Record, "mute", value, i);
		}
	if (nunits[masterPlayMute])
		for (i=0; i<nunits[masterPlayMute]; i++) {
			value[0] = 0;			/* set master Play Mute OFF K.Okamoto */
			setcontrol(Play, "mute", value, i);
		}
	if (nunits[masterRecAGC])
		for (i=0; i<nunits[masterRecAGC]; i++) {
			value[0]=1;						/* set AGC ON, K.Okamoto */
			setcontrol(Record, "agc", value, i);		/* K.Okamoto */
		}
	if (nunits[LRRecVol])
		for (i=0; i<nunits[LRRecVol]; i++) {
			if (argc >=1 && *p == '%') {
				for (j = 0; j < 9; j++) {
					volume[j] = mastervol*(units[LRRecVol][i].max - units[LRRecVol][i].min)/100	+ units[LRRecVol][i].min;
					if (debug &  Dbgcontrol) fprint(2, "Unit#%d's ,  c->max, c->min= 0x%lx, 0x%lx\n", 
						units[LRRecVol][i].id, units[LRRecVol][i].max, units[LRRecVol][i].min);
					if (debug & Dbgcontrol) fprint(2, "volume[%d] = 0x%lx\n", j, volume[j]);
				}
			} else {
				for (j = 0; j < 9; j++) {
					volume[j] = 50*(units[LRRecVol][i].max - units[LRRecVol][i].min)/100	+ 
						units[LRRecVol][i].min;
					if (debug & Dbgcontrol) fprint(2, "Unit#%d's ,  c->max, c->min= 0x%lx, 0x%lx\n", 
						units[LRRecVol][i].id, units[LRRecVol][i].max, units[LRRecVol][i].min);
					if (debug & Dbgcontrol) fprint(2, "volume[%d] = 0x%lx\n", j, volume[j]);
				}
			}
			if (units[LRRecVol][i].settable)
				setcontrol(Record, "volume", volume, i);
		}
	if (nunits[LRPlayVol])
		for (i=0; i<nunits[LRPlayVol]; i++) {
			if (argc >=1 && *p == '%') {
				for (j = 0; j < 9; j++) {
					volume[j] = mastervol*(units[LRPlayVol][i].max - units[LRPlayVol][i].min)/100	+ 
						units[LRPlayVol][i].min;
					if (debug & Dbgcontrol) fprint(2, "Unit#%d's ,  c->max, c->min= 0x%lx, 0x%lx\n", 
						units[LRPlayVol][i].id, units[LRPlayVol][i].max, units[LRPlayVol][i].min);
					if (debug & Dbgcontrol) fprint(2, "volume[%d] = 0x%lx\n", j, volume[j]);
				}
			} else {
				for (j = 0; j < 9; j++) {
					volume[j] = 50*(units[LRPlayVol][i].max - units[LRPlayVol][i].min)/100	+ 
						units[LRPlayVol][i].min;
					if (debug & Dbgcontrol) fprint(2, "Unit#%d's ,  c->max, c->min= 0x%lx, 0x%lx\n", 
						units[LRPlayVol][i].id, units[LRPlayVol][i].max, units[LRPlayVol][i].min);
					if (debug & Dbgcontrol) fprint(2, "volume[%d] = 0x%lx\n", j, volume[j]);
				}
			}
			if (units[LRPlayVol][i].settable)
				setcontrol(Play, "volume", volume, i);
		}
	if (nunits[masterPlayVol])
		for (i=0; i<nunits[masterPlayVol]; i++) {
			if (argc >=1)
				volume[0] = mastervol*(masterplayvol.max - masterplayvol.min)/100+ masterplayvol.min;
			else
				volume[0] = 50*(masterplayvol.max - masterplayvol.min)/100+ masterplayvol.min;
			volume[1] = 0;
			if (debug & Dbgcontrol) fprint(2, "masterplayvol.max = %ld\n", masterplayvol.max);
			if (debug & Dbgcontrol) fprint(2, "masterplayvol.min = %ld\n", masterplayvol.min);
			if (debug & Dbgcontrol) fprint(2, "master Play volume[0] = %ld\n", volume[0]);
			if (units[masterPlayVol][i].settable)
				setcontrol(Play, "volume", volume, i);
		}
	if (buttonendpt > 0){
		sprint(buf, "ep %d bulk r 1 1", buttonendpt);
		if (debug) fprint(2, "sending `%s' to /dev/usb%d/%d/ctl\n", buf, ad->ctlrno, id);
		if (write(ad->ctl, buf, strlen(buf)) > 0)
			proccreate(buttonproc, nil, STACKSIZE);
		else
			fprint(2, "Could not configure button endpoint: %r\n");
	}
	proccreate(controlproc, nil, STACKSIZE);
	proccreate(serve, nil, STACKSIZE);

	threadexits(nil);
}
