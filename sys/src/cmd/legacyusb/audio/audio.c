/*
 * USB audio driver for Plan 9
 * This needs a full rewrite.
 * As it is, it does not check for all errors,
 * mixes the audio data structures with the usb configuration,
 * may cross nil pointers, and is hard to debug and fix.
 * Also, it does not issue a dettach request to the endpoint
 * after the device is unplugged. This means that the old
 * endpoint would still be around until manually reclaimed.
 */

#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "audio.h"
#include "audioctl.h"

#define STACKSIZE 16*1024

extern char* srvpost;
char * mntpt;

Channel *controlchan;

int verbose;
int setrec = 0;
int defaultspeed[2] = {44100, 44100};
Dev *buttondev;
Dev *epdev[2];

static void
audio_endpoint(Dev *, Desc *dd)
{
	byte *b = (uchar*)&dd->data;
	int n = dd->data.bLength;
	char *hd;

	switch(b[2]){
	case 0x01:
		if(usbdebug){
			fprint(2, "CS_ENDPOINT for attributes 0x%x, lockdelayunits %d, lockdelay %#ux, ",
				b[3], b[4], b[5] | (b[6]<<8));
			if(b[3] & has_setspeed)
				fprint(2, "has sampling-frequency control");
			else
				fprint(2, "does not have sampling-frequency control");
			if(b[3] & 0x1<<1)
				fprint(2, ", has pitch control");
			else
				fprint(2, ", does not have pitch control");
			if(b[3] & 0x1<<7)
				fprint(2, ", max packets only");
			fprint(2, "\n");
		}
		if(dd->conf == nil)
			sysfatal("conf == nil");
		if(dd->iface == nil)
			sysfatal("iface == nil");
		if(dd->altc == nil)
			sysfatal("alt == nil");
		if(dd->altc->aux == nil)
			dd->altc->aux= mallocz(sizeof(Audioalt),1);
		((Audioalt*)dd->altc->aux)->caps |= b[3];
		break;
	case 0x02:
		if(usbdebug){
			fprint(2, "CS_INTERFACE FORMAT_TYPE %d, channels %d, subframesize %d, resolution %d, freqtype %d, ",
				b[3], b[4], b[5], b[6], b[7]);
			fprint(2, "freq0 %d, freq1 %d\n",
				b[8] | (b[9]<<8) | (b[10]<<16), b[11] | (b[12]<<8) | (b[13]<<16));
		}
		break;
	default:
		if(usbdebug){
			hd = hexstr(b, n);
			fprint(2, "CS_INTERFACE: %s\n", hd);
			free(hd);
		}
	}
}

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
		if(nf < 3)
			sysfatal("controlproc: not enough arguments");
		replchan = (Channel*)strtol(args[0], nil, 0);
		if(strcmp(args[2], "playback") == 0)
			rec = Play;
		else if(strcmp(args[2], "record") == 0)
			rec = Record;
		else{
			/* illegal request */
			dprint(2, "%s must be record or playback", args[2]);
			if(replchan) chanprint(replchan, "%s must be record or playback", args[2]);
			free(req);
			continue;
		}
		c = nil;
		for(i = 0; i < Ncontrol; i++){
			c = &controls[rec][i];
			if(strcmp(args[1], c->name) == 0)
				break;
		}
		if(i == Ncontrol){
			dprint(2, "Illegal control name: %s", args[1]);
			if(replchan) chanprint(replchan, "Illegal control name: %s", args[1]);
		}else if(!c->settable){
			dprint(2, "%s %s is not settable", args[1], args[2]);
			if(replchan)
				chanprint(replchan, "%s %s is not settable", args[1], args[2]);
		}else if(nf < 4){
			dprint(2, "insufficient arguments for %s %s", args[1], args[2]);
			if(replchan)
				chanprint(replchan, "insufficient arguments for %s %s",
					args[1], args[2]);
		}else if(ctlparse(args[3], c, value) < 0){
			if(replchan)
				chanprint(replchan, "parse error in %s %s", args[1], args[2]);
		}else{
			dprint(2, "controlproc: setcontrol %s %s %s\n",
					rec?"in":"out", args[1], args[3]);
			if(setcontrol(rec, args[1], value) < 0){
				if(replchan)
					chanprint(replchan, "setting %s %s failed", args[1], args[2]);
			}else{
				if(replchan) chanprint(replchan, "ok");
			}
			ctlevent();
		}
		free(req);
	}
}

void
buttonproc(void *)
{
	int	i, fd, b;
	char err[32];
	byte buf[1];
	Audiocontrol *c;

	fd = buttondev->dfd;

	c = &controls[Play][Volume_control];
	for(;;){
		if((b = read(fd, buf, 1)) < 0){
			rerrstr(err, sizeof err);
			if(strcmp(err, "interrupted") == 0){
				dprint(2, "read interrupted\n");
				continue;
			}
			sysfatal("read %s/data: %r", buttondev->dir);
		}
		if(b == 0 || buf[0] == 0){
			continue;
		}else if(buf[0] == 1){
			if(c->chans == 0)
				c->value[0] += c->step;
			else
				for(i = 1; i < 8; i++)
					if(c->chans & 1 << i)
						c->value[i] += c->step;
			chanprint(controlchan, "0 volume playback %A", c);
		}else if(buf[0] == 2){
			if(c->chans == 0)
				c->value[0] -= c->step;
			else
				for(i = 1; i < 8; i++)
					if(c->chans & 1 << i)
						c->value[i] -= c->step;
			chanprint(controlchan, "0 volume playback %A", c);
		}else if(usbdebug){
			fprint(2, "button");
			for(i = 0; i < b; i++)
				fprint(2, " %#2.2x", buf[i]);
			fprint(2, "\n");
		}
	}
}


void
usage(void)
{
	fprint(2, "usage: usbaudio [-dpV] [-N nb] [-m mountpoint] [-s srvname] "
		"[-v volume] [dev]\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char **argv)
{
	char *devdir;
	int i;
	long value[8], volume[8];
	Audiocontrol *c;
	char *p;
	extern int attachok;
	Ep *ep;
	int csps[] = { Audiocsp, 0};

	devdir = nil;
	volume[0] = Undef;
	for(i = 0; i<8; i++)
		value[i] = 0;
	fmtinstall('A', Aconv);
	fmtinstall('U', Ufmt);
	quotefmtinstall();

	ARGBEGIN{
	case 'N':
		p = EARGF(usage());	/* ignore dev nb */
		break;
	case 'd':
		usbdebug++;
		verbose++;
		break;
	case 'm':
		mntpt = EARGF(usage());
		break;
	case 'p':
		attachok++;
		break;
	case 's':
		srvpost = EARGF(usage());
		break;
	case 'v':
		volume[0] = strtol(EARGF(usage()), &p, 0);
		for(i = 1; i < 8; i++)
			volume[i] = volume[0];
		break;
	case 'V':
		verbose++;
		break;
	default:
		usage();
	}ARGEND
	switch(argc){
	case 0:
		break;
	case 1:
		devdir = argv[0];
		break;
	default:
		usage();
	}
	if(devdir == nil)
		if(finddevs(matchdevcsp, csps, &devdir, 1) < 1){
			fprint(2, "No usb audio\n");
			threadexitsall("usbaudio not found");
		}
	ad = opendev(devdir);
	if(ad == nil)
		sysfatal("opendev: %r");
	if(configdev(ad) < 0)
		sysfatal("configdev: %r");
	
	for(i = 0; i < nelem(ad->usb->ddesc); i++)
		if(ad->usb->ddesc[i] != nil)
		switch(ad->usb->ddesc[i]->data.bDescriptorType){
		case AUDIO_INTERFACE:
			audio_interface(ad, ad->usb->ddesc[i]);
			break;
		case AUDIO_ENDPOINT:
			audio_endpoint(ad, ad->usb->ddesc[i]);
			break;
		}

	controlchan = chancreate(sizeof(char*), 8);

	for(i = 0; i < nelem(ad->usb->ep); i++)
		if((ep = ad->usb->ep[i]) != nil){
			if(ep->iface->csp == CSP(Claudio, 2, 0) && ep->dir == Eout)
				endpt[0] = ep->id;
			if(ep->iface->csp == CSP(Claudio, 2, 0) && ep->dir == Ein)
				endpt[1] = ep->id;
			if(buttonendpt<0 && Class(ep->iface->csp) == Clhid)
				buttonendpt = ep->id;
		}
	if(endpt[0] != -1){
		if(verbose)
			fprint(2, "usb/audio: playback on ep %d\n", endpt[0]);
		interface[0] = ad->usb->ep[endpt[0]]->iface->id;
	}
	if(endpt[1] != -1){
		if(verbose)
			fprint(2, "usb/audio: record on ep %d\n", endpt[0]);
		interface[1] = ad->usb->ep[endpt[1]]->iface->id;
	}
	if(verbose && buttonendpt >= 0)
		fprint(2, "usb/audio: buttons on ep %d\n", buttonendpt);

	if(endpt[Play] >= 0){
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
		if(setcontrol(Play, "channels", value) == Undef)
			sysfatal("Can't set play channels");
		value[0] = 16;
		if(setcontrol(Play, "resolution", value) == Undef)
			sysfatal("Can't set play resolution");
	}

	if(endpt[Record] >= 0){
		setrec = 1;
		if(verbose)
			fprint(2, "Setting default record parameters: "
				"%d Hz, %d channels at %d bits\n",
				defaultspeed[Record], 2, 16);
		i = 2;
		while(findalt(Record, i, 16, defaultspeed[Record]) < 0)
			if(i == 2 && controls[Record][Channel_control].max == 1){
				fprint(2, "Warning, can't configure stereo "
					"recording, configuring mono instead\n");
				i = 1;
			}else
				break;
		if(findalt(Record, i, 16, 48000) < 0){
			endpt[Record] = -1;	/* disable recording */
			setrec = 0;
			fprint(2, "Warning, can't configure record for %d Hz or %d Hz\n",
				defaultspeed[Record], 48000);
		}else
			fprint(2, "Warning, can't configure record for %d Hz, "
				"configuring for %d Hz instead\n",
				defaultspeed[Record], 48000);
		defaultspeed[Record] = 48000;
		if(setrec){
			value[0] = i;
			if(setcontrol(Record, "channels", value) == Undef)
				sysfatal("Can't set record channels");
			value[0] = 16;
			if(setcontrol(Record, "resolution", value) == Undef)
				sysfatal("Can't set record resolution");
		}
	}

	getcontrols();	/* Get the initial value of all controls */
	value[0] = defaultspeed[Play];
	if(endpt[Play] >= 0 && setcontrol(Play, "speed", value) < 0)
		sysfatal("can't set play speed");
	value[0] = defaultspeed[Record];
	if(endpt[Record] >= 0 && setcontrol(Record, "speed", value) < 0)
		fprint(2, "%s: can't set record speed\n", argv0);
	value[0] = 0;
	setcontrol(Play, "mute", value);

	if(volume[0] != Undef){
		c = &controls[Play][Volume_control];
		if(*p == '%' && c->min != Undef)
			for(i = 0; i < 8; i++)
				volume[i] = (volume[i]*c->max + (100-volume[i])*c->min)/100;
		if(c->settable)
			setcontrol(Play, "volume", volume);
		c = &controls[Record][Volume_control];
		if(c->settable && setrec)
			setcontrol(Record, "volume", volume);
	}

	if(buttonendpt > 0){
		buttondev = openep(ad, buttonendpt);
		if(buttondev == nil)
			sysfatal("openep: buttons: %r");
		if(opendevdata(buttondev, OREAD) < 0)
			sysfatal("open buttons fd: %r");
		proccreate(buttonproc, nil, STACKSIZE);
	}
	proccreate(controlproc, nil, STACKSIZE);
	proccreate(serve, nil, STACKSIZE);

	threadexits(nil);
}
