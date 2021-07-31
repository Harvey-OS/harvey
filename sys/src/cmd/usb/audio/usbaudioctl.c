#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbaudio.h"
#include "usbaudioctl.h"

int endpt[2] =		{-1, -1};
int interface[2] =	{-1, -1};
int featureid[2] =	{-1, -1};		/* current feature ID for Record or Play */
int selectorid[2] =	{-1, -1};
int mixerid =	-1;				/* store MIxer ID K.Okamoto */
int selector = -1;				/* Selector ID K.Okamoto */
extern int vendor;
extern int product;
int curalt[2] =		{-1, -1};
int buttonendpt =	-1;
extern FeatureAttr units[8][16];		/* K.Okamoto */
extern int nunits[8];		/* K.Okamoto */
MasterVol masterplayvol, masterrecvol;	/* K.Okamoto */

int id;
Device *ad;

Audiocontrol controls[2][Ncontrol] = {
	{
	[Speed_control] = {		"speed",		0, {0}, 0,	44100,	Undef},	/*Play */
	[Mute_control] = {		"mute",		0, {0}, 0,	0,		Undef},
	[Volume_control] = {	"volume",		0, {0}, 0,	0,		Undef},
	[Bass_control] = {		"bass",		0, {0}, 0,	0,		Undef},
	[Mid_control] = {		"mid",		0, {0}, 0,	0,		Undef},
	[Treble_control] = {		"treble",		0, {0}, 0,	0,		Undef},
	[Equalizer_control] = {	"equalizer",	0, {0}, 0,	0,		Undef},
	[Agc_control] = {		"agc",		0, {0}, 0,	0,		Undef},
	[Delay_control] = {		"delay",		0, {0}, 0,	0,		Undef},
	[Bassboost_control] = {	"bassboost",	0, {0}, 0,	0,		Undef},
	[Loudness_control] = {	"loudness",	0, {0}, 0,	0,		Undef},
	[Channel_control] = {	"channels",	0, {0}, 0,	2,		Undef},
	[Resolution_control] = {	"resolution",	0, {0}, 0,	16,		Undef},
	}, {
	[Speed_control] = {		"speed",		0, {0}, 0,	44100,	Undef},	/* Record */
	[Mute_control] = {		"mute",		0, {0}, 0,	0,		Undef},
	[Volume_control] = {	"volume",		0, {0}, 0,	0,		Undef},
	[Bass_control] = {		"bass",		0, {0}, 0,	0,		Undef},
	[Mid_control] = {		"mid",		0, {0}, 0,	0,		Undef},
	[Treble_control] = {		"treble",		0, {0}, 0,	0,		Undef},
	[Equalizer_control] = {	"equalizer",	0, {0}, 0,	0,		Undef},
	[Agc_control] = {		"agc",		0, {0}, 0,	0,		Undef},
	[Delay_control] = {		"delay",		0, {0}, 0,	0,		Undef},
	[Bassboost_control] = {	"bassboost",	0, {0}, 0,	0,		Undef},
	[Loudness_control] = {	"loudness",	0, {0}, 0,	0,		Undef},
	[Channel_control] = {	"channels",	0, {0}, 0,	2,		Undef},
	[Resolution_control] = {	"resolution",	0, {0}, 0,	16,		Undef},
	}
};

int
setaudioalt(int rec, Audiocontrol *c, int control)
{
	if (debug & Dbgcontrol)
		fprint(2, "setcontrol %s: Set alt %d\n", c->name, control);
	curalt[rec] = control;
	if (setupcmd(ad->ep[0], RH2D|Rstandard|Rinterface, SET_INTERFACE, control, interface[rec], nil, 0) < 0){
		if (debug & Dbgcontrol) fprint(2, "setcontrol: setupcmd %s failed\n", c->name);
			return -1;
	}
	return control;
}

int
findalt(int rec, int nchan, int res, int speed)	/* only for USB Streaming Terminal Units K.Okamoto */
{
	Endpt *ep;
	Audioalt *a;
	Dalt *da;
	int i, j, k, retval;

	retval = -1;
	controls[rec][Channel_control].min = 1000000;
	controls[rec][Channel_control].max = 0;
	controls[rec][Channel_control].step = Undef;
	controls[rec][Resolution_control].min = 1000000;
	controls[rec][Resolution_control].max = 0;
	controls[rec][Resolution_control].step = Undef;
	for (i = 0; i < Nendpt; i++) {
		if ((ep = ad->ep[i]) == nil)
			continue;
		if(ep->csp != CSP(CL_AUDIO, 2, 0))
			continue;
		if (ep->iface == nil) {
			fprint(2, "\tno interface\n");
			return 0;
		}
		if ((rec == Play && (ep->addr &  0x80))			/* K.Okamoto */
		|| (rec == Record && (ep->addr &  0x80) == 0))		/* K.Okamoto */
			continue;
		for (j = 0; j < 16; j++) {
			if ((da = ep->iface->dalt[j]) == nil || (a = da->devspec) == nil)
				continue;
			if (a->nchan < controls[rec][Channel_control].min)
				controls[rec][Channel_control].min = a->nchan;
			if (a->nchan > controls[rec][Channel_control].max)
				controls[rec][Channel_control].max = a->nchan;
			if (a->res < controls[rec][Resolution_control].min)
				controls[rec][Resolution_control].min = a->res;
			if (a->res > controls[rec][Resolution_control].max)
				controls[rec][Resolution_control].max = a->res;
			controls[rec][Channel_control].settable = 1;
			controls[rec][Channel_control].readable = 1;
			controls[rec][Resolution_control].settable = 1;
			controls[rec][Resolution_control].readable = 1;
			controls[rec][Speed_control].settable = 1;
			controls[rec][Speed_control].readable = 1;
			if (vendor == 0xd8c && product== 0x6) {	/* K.Okamoto for C-Media CM-106/F chip */
				if (a->res == res){			
					if(speed == Undef)
						retval = j;
					else if(a->caps & (has_discfreq|onefreq)){
						for(k = 0; k < nelem(a->freqs); k++){
							if(a->freqs[k] == speed){
								retval = j;
								break;
							}
						}
					} else {
						if(speed >= a->minfreq && speed <= a->maxfreq)
							retval = j;
					}
				}
			} else {
				if (a->nchan == nchan && a->res == res){
					if(speed == Undef)
						retval = j;
					else if(a->caps & (has_discfreq|onefreq)){
						for(k = 0; k < nelem(a->freqs); k++){
							if(a->freqs[k] == speed){
								retval = j;
								break;
							}
						}
					} else {
						if(speed >= a->minfreq && speed <= a->maxfreq)
							retval = j;
					}
				}
			}
		}
	}
	if ((debug & Dbgcontrol) && retval < 0){
		fprint(2, "findalt(%d, %d, %d, %d) failed\n", rec, nchan, res, speed);
	}
	return retval;
}

int
setspeed(int rec, int speed)	/* only for USB Streaming Terminal Units K.Okamoto */
{
	int ps, n, no;
	Audioalt *a;
	Dalt *da;
	Endpt *ep;
	char cmdbuf[32];
	uchar buf[3];

	if (curalt[rec] < 0){
		fprint(2, "Must set channels and resolution before speed\n");
		return Undef;
	}
	if (endpt[rec] < 0)
		sysfatal("endpt[%s] not set\n", rec?"Record":"Playback");
	ep = ad->ep[endpt[rec]];
	if (ep->iface == nil)
		sysfatal("no interface");
	if (curalt[rec] < 0)
		sysfatal("curalt[%s] not set\n", rec?"Record":"Playback");
	da = ep->iface->dalt[curalt[rec]];
	a = da->devspec;
	no = -1;
	if (a->caps & onefreq){
		speed = a->freqs[0];		/* speed not settable, but packet size must still be set */
	}else if (a->caps & has_contfreq){
		if (speed < a->minfreq)
			speed = a->minfreq;
		else if (speed > a->maxfreq)
			speed = a->maxfreq;
		if (debug & Dbgcontrol)
			fprint(2, "Setting continuously variable %s speed to %d\n",
				rec?"record":"playback", speed);
	}else if (a->caps & has_discfreq){
		int dist, i;
		dist = 1000000;
		no = -1;
		for (i = 0; a->freqs[i] > 0; i++) {
			if (abs(a->freqs[i] - speed) < dist){
				dist = abs(a->freqs[i] - speed);
				no = i;
			}
		}
		if (no == -1){
			if (debug & Dbgcontrol)
				fprint(2, "no = -1\n");
			return Undef;
		}
		speed = a->freqs[no];
		if (debug & Dbgcontrol)
			fprint(2, "Setting discreetly variable %s speed to %d\n",
				rec?"record":"playback", speed);
	}else{
		if (debug & Dbgcontrol)
			fprint(2, "can't happen\n?");
		return Undef;
	}
	if (a->caps & has_setspeed){
		if (debug & Dbgcontrol)
			fprint(2, "Setting %s speed to %d Hz;",
				rec?"record":"playback", speed);
		if (a->caps & has_discfreq){
//			if ((vendor == 0xd8c && product== 0x6) 	/* K.Okamoto for C-Media CM-106/F chip */
//				|| (vendor == 0x746 && product == 0x5502)) {		/* for Onkyo SE-U55X */
			if (vendor == 0xd8c && product== 0x6) {	/* K.Okamoto for C-Media CM-106/F chip */
				buf[0] = speed;
				buf[1] = speed >> 8;
				buf[2] = speed >> 16;
			} else {
				buf[0] = no;
				buf[1] = 0;
				buf[2] = 0;
			}
		}else{
			buf[0] = speed;
			buf[1] = speed >> 8;
			buf[2] = speed >> 16;
		}
		if (setupcmd(ad->ep[0], RH2D|Rclass|Rendpt, SET_CUR, sampling_freq_control<<8, endpt[rec], buf, 3) < 0)
			return Undef;
		if (setupreq(ad->ep[0], RD2H|Rclass|Rendpt, GET_CUR, sampling_freq_control<<8, endpt[rec], 3) < 0)
			return Undef;
		n = setupreply(ad->ep[0], buf, 3);
		if (n != 3)
			return Undef;
		if (buf[2]){					/* K.Okamoto temporal remove, because C-106F returns all 0 */
			if (debug & Dbgcontrol)
				fprint(2, "Speed out of bounds %d (0x%x)\n",
					buf[0] | buf[1] << 8 | buf[2] << 16,
					buf[0] | buf[1] << 8 | buf[2] << 16);
		}else
			speed = buf[0] | buf[1] << 8 | buf[2] << 16;
		if (debug & Dbgcontrol)
			fprint(2, " speed now %d Hz;", speed);
	}
	ps = ((speed * da->interval + 999) / 1000)
		* controls[rec][Channel_control].value[0]
		* controls[rec][Resolution_control].value[0]/8;
	if(ps > ep->maxpkt){
		fprint(2, "packet size %d > maximum packet size %d",
			ps, ep->maxpkt);
		return Undef;
	}
	if (debug & Dbgcontrol)
		fprint(2, "Configuring %s endpoint for %d Hz\n",
				rec?"record":"playback", speed);
//	if (controls[rec][Channel_control].value[0] == 8)	/* K.Okamoto for 8 channel -> 2 channel */
//		controls[rec][Channel_control].value[0] /= 4;
	sprint(cmdbuf, "ep %d %d %c %ld %d", endpt[rec], da->interval, rec?'r':'w', 
	controls[rec][Channel_control].value[0]*controls[rec][Resolution_control].value[0]/8,
	speed);
	if (write(ad->ctl, cmdbuf, strlen(cmdbuf)) != strlen(cmdbuf)){
		fprint(2, "writing %s to #U/usb%d/%d/ctl: %r\n", cmdbuf, ad->ctlrno, id);
		return Undef;
	}
	if (debug & Dbgcontrol) fprint(2, "sent `%s' to /dev/usb/%d/ctl\n", cmdbuf, id);
	if (da->attrib & 0x8)	{	/* Adaptive Isochoronous transfer for Play, K.Okamoto*/
		/* program to set the endpoint to no Adaptive but Synchrounous K.Okamoto */
		buf[0] = 1;		/* TRUE */
		if (setupcmd(ad->ep[0], RH2D|Rclass|Rendpt, SET_CUR, pitch_control<<8, endpt[rec], buf, 1) < 0)
			return Undef;
		if (setupreq(ad->ep[0], RD2H|Rclass|Rendpt, GET_CUR, pitch_control<<8, endpt[rec], 1) < 0)
			return Undef;
		n = setupreply(ad->ep[0], buf, 1);
		if (n == 1)
			if (verbose) fprint(2, "Pitch control = %s\n", buf[0]?"ON":"OFF");
	}
	return speed;
}

long
getspeed(int rec, int which)	/* only for USB Streaming Terminal Units K.Okamoto */
{
	int i, n;
	Audioalt *a;
	Dalt *da;
	Endpt *ep;
	uchar buf[3];

	if (curalt[rec] < 0){
		fprint(2, "Must set channels and resolution before getspeed\n");
		return Undef;
	}
	if (endpt[rec] < 0)
		sysfatal("endpt[%s] not set\n", rec?"Record":"Playback");
	ep = ad->ep[endpt[rec]];
	if (ep->iface == nil)
		sysfatal("no interface");
	if (curalt[rec] < 0)
		sysfatal("curalt[%s] not set\n", rec?"Record":"Playback");
	da = ep->iface->dalt[curalt[rec]];
	a = da->devspec;
	if (a->caps & onefreq){
		if (which == GET_RES)
			return Undef;
		return a->freqs[0];		/* speed not settable */
	}else if (a->caps & has_setspeed){
		if (setupreq(ad->ep[0], RD2H|Rclass|Rendpt, which, sampling_freq_control<<8, endpt[rec], 3) < 0)
			return Undef;
		n = setupreply(ad->ep[0], buf, 3);
		if (n != 3)
			return Undef;
		if (buf[2]){
			if (debug & Dbgcontrol)
				fprint(2, "Speed out of bounds\n");
			if ((a->caps & has_discfreq) && (buf[0] | buf[1] << 8) < 8)
				return a->freqs[buf[0] | buf[1] << 8];
		}
		return buf[0] | buf[1] << 8 | buf[2] << 16;
	}else if (a->caps & has_contfreq){
		if (which == GET_CUR)
			return controls[rec][Speed_control].value[0];
		if (which == GET_MIN)
			return a->minfreq;
		if (which == GET_MAX)
			return a->maxfreq;
		if (which == GET_RES)
			return 1;
	}else if (a->caps & has_discfreq){
		if (which == GET_CUR)
			return controls[rec][Speed_control].value[0];
		if (which == GET_MIN)
			return a->freqs[0];
		for (i = 0; i < 8 && a->freqs[i] > 0; i++)
			;
		if (which == GET_MAX)
			return a->freqs[i-1];
		if (which == GET_RES)
			return Undef;
	}
	if (debug & Dbgcontrol)
		fprint(2, "can't happen\n?");
	return Undef;
}

/* setcontrol should be called by the Unit number (int unit) if only one Unit must be there,
    now, those are speed, channel, resolution controls.   Otherwise, the "int unit" argument
indicate the turn number of multiple Units vector,  K.Okamoto  */
int
setcontrol(int rec, char *name, long *value, int unit)
{
	int i, j, ctl, m;
	int type, req, control, index, count;
	Audiocontrol *c;

	c = nil;
	for (ctl = 0; ctl < Ncontrol; ctl++){
		c = &controls[rec][ctl];
		if (strcmp(name, c->name) == 0)
			break;
	}
	if (ctl == Ncontrol){
		if (debug & Dbgcontrol) fprint(2, "setcontrol: control not found\n");
		return -1;
	}
	if (c->settable == 0) {
		if (debug & Dbgcontrol) fprint(2, "setcontrol: control %d.%d not settable\n", rec, ctl);
		if (c->chans){
			for (i = 0; i < 9; i++)		/* K.Okamoto */
				if ((c->chans & 1 << (i-1)) && c->value[i] != value[i])	/* K.Okamoto */
					return -1;
			return 0;
		}
		if (c->value[0] != value[0]) {
			if (debug & Dbgcontrol)
				fprint(2, "setcontrol: c->value[0] = %d, value[0] = %d\n", (int)c->value[0], (int)value[0]);
			return -1;
		}
		return 0;
	}
	if (c->chans){		/* not master channel */
		value[0] = 0;	// set to average
		m = 0;
		for (i = 1; i < 9; i++)		/* K.Okamoto */
			if (c->chans & 1 << (i-1)){	/* K.Okamoto */
				if (c->min != Undef && value[i] < c->min)
					value[i] = c->min;
				if (c->max != Undef && value[i] > c->max)
					value[i] = c->max;
				value[0] += value[i];
				m++;
			} else
				value[i] = Undef;
		if (m) value[0] /= m;
	}else{
		if (c->min != Undef && value[0] < c->min)
			value[0] = c->min;
		if (c->max != Undef && value[0] > c->max)
			value[0] = c->max;
	}
	req = SET_CUR;
	count = 1;
	switch(ctl){
	default:
		if (debug & Dbgcontrol) fprint(2, "setcontrol: can't happen\n");
		return -1;
	case Speed_control:
		if ((value[0] = setspeed(rec, value[0])) < 0)
			return -1;
		c->value[0] = value[0];
		return 0;
	case Equalizer_control:
		/* not implemented */
		return -1;
	case Resolution_control:
		control = findalt(rec, controls[rec][Channel_control].value[0], value[0], defaultspeed[rec]);
		if(control < 0 || setaudioalt(rec, c, control) < 0){
			if (debug & Dbgcontrol) fprint(2, "setcontrol: can't find setting for %s\n",
				c->name);
			return -1;
		}
		c->value[0] = value[0];
		controls[rec][Speed_control].value[0] = defaultspeed[rec];
		return 0;
	case Volume_control:
		type = RH2D|Rclass|Rinterface;
		count = 2;
		control = ctl<<8;
//		c->min = -32767;		/* K.Okamoto */
//		c->max = 32767;		/* K.Okamoto */
//		c->step = 0xff;		/* K.Okamoto */
		if (rec == Play) {
			if (value[1] == 0) {
				featureid[rec] = units[masterPlayVol][unit].id;			/* K.Okamoto */
				index = units[masterPlayVol][unit].id<<8;					/* K.Okamoto */
				if (verbose)
					fprint(2, "setcontrol: PlayVolume Unit = %d\n", 
						units[masterPlayVol][unit].id);	/* K.Okamoto */
				c->readable = units[masterPlayVol][unit].readable;
				c->settable = units[masterPlayVol][unit].settable;
				c->chans = units[masterPlayVol][unit].chans;
				for(j=0; j<9; j++)
					c->value[j] = units[masterPlayVol][unit].value[j];
				c->min = units[masterPlayVol][unit].min;
				c->max = units[masterPlayVol][unit].max;
				c->step = units[masterPlayVol][unit].step;
				if (setmasterValue(c, type, req, control, index, count, value) < 0) {
					if (verbose) fprint(2, "failed to set master Play Volume for  Unit %d\n", units[masterPlayVol][unit].id);
					return -1;
				}
				units[masterPlayVol][unit].value[0] = value[0];
				c->value[0] = value[0];		// redandant K.Okamoto
				if (verbose)
					fprint(2, "master Volume value = %d of Units %d\n", (int)value[0], units[masterPlayVol][unit].id);
			} else {
				featureid[rec] = units[LRPlayVol][unit].id;			/* K.Okamoto */
				index = units[LRPlayVol][unit].id<<8;					/* K.Okamoto */
				if (verbose)
					fprint(2, "setcontrol: PlayVolume Unit = %d\n", 
						units[LRPlayVol][unit].id);	/* K.Okamoto */
				c->readable = units[LRPlayVol][unit].readable;
				c->settable = units[LRPlayVol][unit].settable;
				c->chans = units[LRPlayVol][unit].chans;
				c->min = units[LRPlayVol][unit].min;
				c->max = units[LRPlayVol][unit].max;
				c->step = units[LRPlayVol][unit].step;
				for(j=0; j<9; j++)
					c->value[j] = units[LRPlayVol][unit].value[j];
				if (setchannelVol(c, type, req, control, index, count, value) < 0) {
					if (verbose) fprint(2, "failed to set LR Play Volume for  Unit %d\n", units[LRPlayVol][unit].id);
					return -1;
				}
				for (j=0; j<9; j++) 
				 	if (value[j]) units[LRPlayVol][unit].value[j] = value[j];
				if (verbose) {
					for(j=0; j<9; j++) fprint(2, "value[%d] = %lx ", j, value[j]);
					fprint(2, "\n");
				}
				for(j=0; j<9; j++)
					units[LRPlayVol][unit].value[j] = c->value[j];
			}
		}
		if (rec == Record) {
			featureid[rec] = units[LRRecVol][unit].id;			/* test only for 0 now K.Okamoto */
			if (verbose)
				fprint(2, "setcontrol: LRVolume Unit = %d\n", units[LRRecVol][unit].id);		/* K.Okamoto */
			index = units[LRRecVol][unit].id<<8;
			c->readable = units[LRRecVol][unit].readable;
			c->settable = units[LRRecVol][unit].settable;
			c->chans = units[LRRecVol][unit].chans;
			c->min = units[LRRecVol][unit].min;
			c->max = units[LRRecVol][unit].max;
			c->step = units[LRRecVol][unit].step;
			for(j=0; j<9; j++)
				c->value[j] = units[LRRecVol][unit].value[j];
			if (verbose) {
				for(j=0; j<9; j++) fprint(2, "value[%d] = %lx ", j, value[j]);
				fprint(2, "\n");
			}
			if (setchannelVol(c, type, req, control, index, count, value) < 0) {
				if (verbose) fprint(2, "failed to set LRRecVolume for  Unit %d\n", units[LRRecVol][unit].id);
				return -1;
			}
			for (j=0; j<9; j++) 
				 if (value[j]) units[LRRecVol][unit].value[j] = value[j];
		}
		return 0;			/* K.Okamoto */
	case Mute_control:		/* K.Okamoto */
		if (req == GET_MIN || req == GET_MAX || req == GET_RES) return 0;
		type = RH2D|Rclass|Rinterface;
		control = ctl<<8;
//		c->min = 0;		/* False, K.Okamoto */
//		c->max = 1;		/* True, K.Okamoto */
//		c->step = 1;		/* K.Okamoto */
		if (rec == Record && !units[masterRecMute][unit].chans){			/* K.Okamoto */
			c->readable = units[masterRecMute][unit].readable;
			c->settable = units[masterRecMute][unit].settable;
			c->chans = units[masterRecMute][unit].chans;
			for(j=0; j<9; j++)
				c->value[j] = units[masterRecMute][unit].value[j];
			c->min = units[masterRecMute][unit].min;
			c->max = units[masterRecMute][unit].max;
			c->step = units[masterRecMute][unit].step;
			featureid[rec] = units[masterRecMute][unit].id;;
			index = units[masterRecMute][unit].id<<8;				/* K.Okamoto */
			if (verbose)
				fprint(2, "setcontrol: Record Mute Unit = %d\n", featureid[rec]);	/* K.Okamoto */
			if (setmasterValue(c, type, req, control, index, count, value) < 0) {
				if (verbose) fprint(2, "failed to set Rec Mute for  Unit %d\n", featureid[rec]);
					return -1;
			}
			units[masterRecMute][unit].value[0] = value[0];
			if (verbose)
				fprint(2, "master Record Mute value = %s of Unit %d\n", value[0]?"True":"False", featureid[rec]);
		} else if (rec == Play && !units[masterPlayMute][unit].chans){
			c->readable = units[masterPlayMute][unit].readable;
			c->settable = units[masterPlayMute][unit].settable;
			c->chans = units[masterPlayMute][unit].chans;
			for(j=0; j<9; j++)
				c->value[j] = units[masterPlayMute][unit].value[j];
			c->min = units[masterPlayMute][unit].min;
			c->max = units[masterPlayMute][unit].max;
			c->step = units[masterPlayMute][unit].step;
			featureid[rec] = units[masterPlayMute][unit].id;
			index = units[masterPlayMute][unit].id<<8;				/* K.Okamoto */
			if (verbose)
				fprint(2, "setcontrol: Play Mute Unit = %d\n", featureid[rec]);	/* K.Okamoto */
			if (setmasterValue(c, type, req, control, index, count, value) < 0) {
				if (verbose) fprint(2, "failed to set master Play Mute for  Unit %d\n", featureid[rec]);
				return -1;
			}
			units[masterPlayMute][unit].value[0] = value[0];
			if (verbose)
				fprint(2, "master Play Mute value = %s of Unit %d\n", value[0]?"True":"False", featureid[rec]);
		}
		if (!controls[rec][Mute_control].value[0] || !(controls[rec][Mute_control].value[0] == 1))
			controls[rec][Mute_control].value[0] = 0;	/* K.Okamoto */
		return 0;
	case Agc_control:
		if (req == GET_MIN || req == GET_MAX || req == GET_RES) return 0;
		count = 1;
		control = ctl<<8;
		if (rec == Record) {
			type = RH2D|Rclass|Rinterface;
			c->readable = units[masterRecAGC][unit].readable;
			c->settable = units[masterRecAGC][unit].settable;
			c->chans = units[masterRecAGC][unit].chans;
			for(j=0; j<9; j++)
				c->value[j] = units[masterRecAGC][unit].value[j];
			c->min = units[masterRecAGC][unit].min;
			c->max = units[masterRecAGC][unit].max;
			c->step = units[masterRecAGC][unit].step;
			if (!controls[rec][Agc_control].chans) {	/* for Channel master Record Mute, K.Okamoto */
//				c->min = 0;		/* False, K.Okamoto */
//				c->max = 1;		/* True, K.Okamoto */
//				c->step = 1;		/* K.Okamoto */
				featureid[rec] = units[masterRecAGC][unit].id;
				index = units[masterRecAGC][unit].id<<8;				/* K.Okamoto */
				if (verbose)
					fprint(2, "setcontrol: Record Agc Unit = %d\n", featureid[rec]);	/* K.Okamoto */
				if (setmasterValue(c, type, req, control, index, count, value) < 0) {
					if (verbose) fprint(2, "failed to set AGC for  Unit %d\n", featureid[rec]);
					return -1;
				}
				units[masterRecAGC][unit].value[0] = value[0];
				if (verbose)
					fprint(2, "master Record AGC value %ld of Unit %d\n", value[0], featureid[rec]);
			}
			return 0;
		} else
			return 0;
	case Channel_control:
		control = findalt(rec, value[0], controls[rec][Resolution_control].value[0], defaultspeed[rec]);
		if(control < 0 || setaudioalt(rec, c, control) < 0){
			if (debug & Dbgcontrol) fprint(2, "setcontrol: can't find setting for %s\n",
				c->name);
			return -1;
		}
		c->value[0] = value[0];
		controls[rec][Speed_control].value[0] = defaultspeed[rec];
		return 0;
	case Delay_control:
		count = 2;
		/* fall through */
	case Bass_control:
	case Mid_control:
	case Treble_control:
	case Bassboost_control:
	case Loudness_control:
		type = RH2D|Rclass|Rinterface;
		control = ctl<<8;
		index = featureid[rec]<<8;
		break;
	}
	if (c->chans)
		if (setchannelVol(c, type, req, control, index, count, value) == -1)
			return -1;
	else
		if (setmasterValue(c, type, req, control, index, count, value) == -1)
			return -1;
	return 0;
}

int
getspecialcontrol(Audiocontrol *c, int rec, int ctl, int req, long *value)
{
	int i, j;
	int type, control, index, count;		/* count: bytes of parameter data block */

	count = 1;
	switch(ctl){
	default:
		return Undef;
	case Speed_control:
		value[0] =  getspeed(rec, req);
		return 0;
	case Channel_control:
	case Resolution_control:
		if (req == GET_MIN)
			value[0] = controls[rec][ctl].min;
		if (req == GET_MAX)
			value[0] = controls[rec][ctl].max;
		if (req == GET_RES)
			value[0] = controls[rec][ctl].step;
		if (req == GET_CUR)
			value[0] = controls[rec][ctl].value[0];
		return 0;
	case Volume_control:
		type = RD2H|Rclass|Rinterface;
		count = 2;
		control = ctl<<8;
		if (rec == Play) {
			for (i=0; i<nunits[masterPlayVol]; i++){
				featureid[rec] = units[masterPlayVol][i].id;			/* K.Okamoto */
				index = units[masterPlayVol][i].id<<8;				/* K.Okamoto */
				value[0] = Undef;
				if (!c->readable) c->readable = 1;
				if (!c->settable) c->settable = 1;
				if (getmasterValue(type, req, control, index, count, value) < 0)
					continue;
				units[masterPlayVol][i].readable = c->readable;
				units[masterPlayVol][i].settable = c->settable;
				units[masterPlayVol][i].chans = c->chans = 0;
				for (j=0; j<9; j++) c->value[j] = units[masterPlayVol][i].value[j];
				if (req == GET_MIN) units[masterPlayVol][i].min = value[0];
				if (req == GET_MAX) units[masterPlayVol][i].max = value[0];
				if (req == GET_RES) units[masterPlayVol][i].step = value[0];
				if (req == GET_CUR) units[masterPlayVol][i].value[0] = value[0];
				if (verbose) {
					fprint(2, "master Play Volume Unit = %d\n", units[masterPlayVol][i].id);	/* K.Okamoto */
					printValue(c, rec, value);
				}
//				if (value[0] != Undef && c->step == Undef) {
//					c->min = -32767;		/* K.Okamoto */
//					c->max = 32767;		/* K.Okamoto */
//					c->step = 0xff;		/* K.Okamoto */
//				}
			}
			for (i=0; i<nunits[LRPlayVol]; i++){
				featureid[rec] = units[LRPlayVol][i].id;			/* K.Okamoto */
				index = units[LRPlayVol][i].id<<8;				/* K.Okamoto */
				value[0] = Undef;
				if (!c->readable) c->readable = 1;
				if (!c->settable) c->settable = 1;
				c->chans = units[LRPlayVol][i].chans;
				if (getchannelVol(type, req, control, index, count, c->chans, value) < 0)
					fprint(2, "failed to get LRPlay Volume #%d\n", featureid[rec]);
				units[LRPlayVol][i].readable = c->readable;
				units[LRPlayVol][i].settable = c->settable;
				if (req == GET_MIN) units[LRPlayVol][i].min = value[0];
				if (req == GET_MAX) units[LRPlayVol][i].max = value[0];
				if (req == GET_RES) units[LRPlayVol][i].step = value[0];
				if (req == GET_CUR)
					for (j=0; j<9; j++) units[LRPlayVol][i].value[j] = value[j];
				if (verbose) {
					fprint(2, "channel Play Volume Unit = %d\n", units[LRPlayVol][i].id);
					printValue(c, rec, value);
				}
			}
		}
		if (rec == Record) {
			for (i=0; i<nunits[LRRecVol]; i++){
				featureid[rec] = units[LRRecVol][i].id;			/* test only for 0 now K.Okamoto */
				if(Dbginfo&Dbgcontrol) fprint(2, "Volume Control Record mode, i = %d\n", units[LRRecVol][i].id);
				index = units[LRRecVol][i].id<<8;
				if (!c->readable) c->readable = 1;
				if (!c->settable) c->settable = 1;
				c->chans = units[LRRecVol][i].chans;
				if (verbose) fprint(2, "unit %d 's chan = %x\n", units[LRRecVol][i].id, c->chans);
				if (getchannelVol(type, req, control, index, count, c->chans, value) < 0)
					fprint(2, "failed to get LRRecord Volume #%d\n", featureid[rec]);
				units[LRRecVol][i].readable = c->readable;
				units[LRRecVol][i].settable = c->settable;
				if ( req == GET_MIN) units[LRRecVol][i].min = value[0];
				if (req == GET_MAX) units[LRRecVol][i].max = value[0];
				if (req == GET_RES) units[LRRecVol][i].step = value[0];
				if (req == GET_CUR)
					for (j=0; j<9; j++) units[LRRecVol][i].value[j] = value[j];
				if (verbose) {
					fprint(2, "Channel Rec Volume Unit = %d\n", featureid[rec]);	/* K.Okamoto */
					printValue(c, req, value);
				}
//				if (value[i] != Undef && units[LRRecVol][i].step == Undef) {
//					units[LRRecVol][i]].min = -32767;		/* K.Okamoto */
//					units[LRRecVol][i].max = 32767;		/* K.Okamoto */
//					units[LRRecVol][i].step = 0xff;		/* K.Okamoto */
//				}
			}
		if (verbose) 
			for (i=0; i<9; i++) fprint(2, "value[%d] = 0x%lx\n", i, value[i]);
		}
		return 0;
	case Mute_control:
		if (req == GET_MIN || req == GET_MAX || req == GET_RES) return 0;
		type = RD2H|Rclass|Rinterface;
		control = ctl<<8;
		if (rec == Record){						/* K.Okamoto */
			for(i=0; i<nunits[masterRecMute]; i++) {
			 	featureid[rec] = units[masterRecMute][i].id;		/* K.Okamoto */
				index = units[masterRecMute][i].id<<8;				/* K.Okamoto */
				units[masterRecMute][i].chans = 0;
				value[0] = Undef;
				if (!c->readable) c->readable = 1;
				if (!c->settable) c->settable = 1;
				if (getmasterValue(type, req, control, index, count, value) < 0)
					value[0] = Undef;
				units[masterRecMute][i].readable = c->readable;
				units[masterRecMute][i].settable = c->settable;
				units[masterRecMute][i].chans = 0;
				if(verbose) {
					fprint(2, "master Rec Mute Unit = %d\n", units[masterRecMute][i].id);	/* K.Okamoto */
					printValue(c, req, value);
				}
//				if (value[0] != Undef && units[masterRecMute][i].step == Undef) {
//					units[masterRecMute][i].min = 0;		/* False, K.Okamoto */
//					units[masterRecMute][i].max = 1;		/* True, K.Okamoto */
//					units[masterRecMute][i].step = 1;		/* K.Okamoto */
//				}
			}
		} else{
			for(i=0; i<nunits[masterPlayMute] ;i++) {
			 	featureid[rec] = units[masterPlayMute][i].id;		/* K.Okamoto */
				index = units[masterPlayMute][i].id<<8;				/* K.Okamoto */
				if (!c->readable) c->readable =1;
				if (!c->settable) c->settable =1;
				value[0] = Undef;
				if (getmasterValue(type, req, control, index, count, value) < 0)
					value[0] = Undef;
				units[masterPlayMute][i].readable = c->readable;
				units[masterPlayMute][i].settable = c->settable;
				units[masterPlayMute][i].chans = 0;
				if (req == GET_MIN) units[masterPlayMute][i].min = value[0];
				if (req == GET_MAX) units[masterPlayMute][i].max = value[0];
				if (req == GET_RES) units[masterPlayMute][i].step = value[0];
				if(verbose) {
					fprint(2, "master Play  Mute Unit = %d\n", units[masterPlayMute][i].id);		/* K.Okamoto */
					printValue(c, req, value);
				}
//				if (value[0] != Undef && units[masterPlayMute][i].step == Undef) {
//					units[masterPlayMute][i].min = 0;		/* False, K.Okamoto */
//					units[masterPlayMute][i].max = 1;		/* True, K.Okamoto */
//					units[masterPlayMute][i].step = 1;		/* K.Okamoto */
//				}
			}
		}
		return 0;
	case Bass_control:
	case Mid_control:
	case Treble_control:
	case Equalizer_control:
	case Agc_control:
		if (req == GET_MIN || req == GET_MAX || req == GET_RES) return 0;
		if (rec == Record) {
			type = RD2H|Rclass|Rinterface;
			control = ctl<<8;
			for(i=0; i<nunits[masterRecAGC] ;i++) {
				featureid[rec] = units[masterRecAGC][i].id;	/* K.Okamoto */
				index = units[masterRecAGC][i].id<<8;			/* K.Okamoto */
				value[0] = Undef;
				if (!c->readable) c->readable = 1;
				if (!c->settable) c->settable = 1;
				if (getmasterValue(type, req, control, index, count, value) < 0)
					value[0] = Undef;
				units[masterRecAGC][i].readable = c->readable;
				units[masterRecAGC][i].settable = c->settable;
				units[masterRecAGC][i].chans = 0;
				if (req == GET_MIN) units[masterRecAGC][i].min = value[0];
				if (req == GET_MAX) units[masterRecAGC][i].max = value[0];
				if (req == GET_RES) units[masterRecAGC][i].step = value[0];
				if(verbose) {
					fprint(2, "master AGC Unit = %d\n", units[masterRecAGC][i].id);		/* K.Okamoto */
					printValue(c, req, value);
				}
//				if (value[0] != Undef && units[masterRecAGC][i].step == Undef) {
//					units[masterRecAGC][i].min = 0;		/* False, K.Okamoto */
//					units[masterRecAGC][i].max = 1;		/* True, K.Okamoto */
//					units[masterRecAGC][i].step = 1;		/* K.Okamoto */
//				}
			}
			return 0;
		} else
			return -1;
		break;
	case Delay_control:
		count = 2;
		/* fall through */
	case Bassboost_control:
	case Loudness_control:
		type = RD2H|Rclass|Rinterface;
		control = ctl<<8;
		index = featureid[rec]<<8;
		break;
	}
	if (controls[rec][ctl].chans){
		getchannelVol(type, req, control, index, count, controls[rec][ctl].chans, value);
		if (verbose) printValue(c, req, value);
	}
	value[0] = Undef;
	if (getmasterValue(type, req, control, index, count, value) < 0)
		value[0] = Undef;
	if (verbose) printValue(c, req, value);
	return 0;
}

int
setmasterValue(Audiocontrol *c, int type, int req, int control, int index, int count, long *value)
{
	byte buf[2];
	int n;
	short svalue;

	switch(count){
	case 2:
		buf[1] = value[0] >> 8;
		buf[0] = value[0] & 0xff;
	case 1:
		buf[0] = value[0];
	}
	if (setupcmd(ad->ep[0], type, req, control, index, buf, count) < 0){
		if (debug & Dbgcontrol) fprint(2, "setcontrol: setupcmd %s failed\n",
			c->name);
		return -1;
	}
	if (setupreq(ad->ep[0], type|0x80, req|0x80, control, index, count) < 0)
		return -1;
	n = setupreply(ad->ep[0], buf, count);
	if (n != count)
		return -1;
	switch (count) {
	case 2:
		svalue = buf[1] << 8 | buf[0];
		value[0] = svalue;
		break;
	case 1:
		value[0] = buf[0];
	}
	c->value[0] = value[0];
	return 0;
}

int setchannelVol(Audiocontrol *c, int type, int req, int control, int index, int count, long *value)
{
	byte buf[2];
	int i, n;
	short svalue;

	if (c->chans){
		for (i = 1; i < 9; i++) {
			if (c->chans & 1 << (i-1)){	/* K.Okamoto */
				switch(count){
				case 2:
					buf[1] = value[i]>>8;
					buf[0] = value[i] & 0xff;
				case 1:
					buf[0] = value[i] & 0xff;
				}
				if (setupcmd(ad->ep[0], type, req, control | i, index, buf, count) < 0){
					if (debug & Dbgcontrol) fprint(2, "setcontrol: setupcmd %s failed\n",
						c->name);
					return -1;
				}
				if (setupreq(ad->ep[0], type|0x80, req|0x80, control|i, index, count) < 0)
					return -1;
				n = setupreply(ad->ep[0], buf, count);
				if (n != count)
					return -1;
				switch (count) {
					case 2:
						svalue = buf[1]<<8 | buf[0];
						value[i] = svalue;
						break;
					case 1:
						value[i] = buf[0];
				}
			}
			c->value[i] = value[i];
		}
	}
	return 0;
}

int
getmasterValue(int type, int req, int control, int index, int count, long *value)
{
	byte buf[2];
	int n;
	short svalue;

	if (setupreq(ad->ep[0], type, req, control, index, count) < 0)
		return -1;
	n = setupreply(ad->ep[0], buf, count);
	if (n != count)
		return -1;
	switch (count) {
	case 2:
		svalue = buf[1] << 8 | buf[0];
		value[0] = svalue;
		break;
	case 1:
		value[0] = buf[0];
	}
	return 0;
}

int
printValue(Audiocontrol *c, int req, long *value)
{
	int i;

	if (verbose){
		if (req == GET_MIN)
			if (value[0] == 0x8000)
				fprint(2, " min = -oo");
			else
				fprint(2, "min = %ld", value[0]);
		else if (req == GET_MAX)
			if (value[0] == 0x8000)
				fprint(2, " max = -oo");
			else
				fprint(2, "max = %ld", value[0]);
		else if (req == GET_RES)
			if (value[0] == 0x8000)
				fprint(2, " step = -oo");
			else
				fprint(2, "step = %ld", value[0]);
		else if (req == GET_CUR){
			if (c->chans){
				fprint(2, "values");
				for (i = 1; i < 9; i++){		/* K.Okamoto */
					if (c->chans & 1 << (i-1))	/* K.Okamoto */
						if (c->value[i] == 0x8000)
							fprint(2, "[%d] %s ", i, "-oo");
						else
							fprint(2, "[%d] %ld  ", i, c->value[i]);
					else
						if (c->value[i] == 0x8000)
							fprint(2, "value %s ", "-oo");
						else
							fprint(2, ", value %ld", c->value[0]);
				}
			} else
				if (c->value[0] == 0x8000)
					fprint(2, "value[0] = %s", "-oo");
				else
					fprint(2, "value[0] = %ld", c->value[0]);
		}
		fprint(2, "\n");
	}
	return 0;
}

int
getchannelVol(int type, int req, int control, int index, int count, uchar chans, long *value)
{
	byte buf[3];
	int i, m, n;
	short svalue;

	m = 0;
	value[0] = 0; // set to average
	for (i = 1; i < 9; i++){		/* K.Okamoto */
		value[i] = Undef;
		if (chans & 1 << (i-1)){		/* K.Okamoto */
			if (setupreq(ad->ep[0], type, req, control | i, index, count) < 0)
				return Undef;
			n = setupreply(ad->ep[0], buf, count);
			if (n != count)
				return -1;
			switch (count) {
			case 2:
				svalue = buf[1]<<8 | buf[0];
				if (req == GET_CUR){
					value[i] = svalue;
					value[0] += svalue;
					m++;
				}else
					value[0] = svalue;
				break;
			case 1:
				if (req == GET_CUR){
					value[i] = buf[0];
					value[0] += buf[0];
					m++;
				}else
					value[0] = buf[0];
			}
		}

	}
	if (m) value[0] /= m;		/* get average */
	return 0;
}

int
getcontrol(int rec, char *name, long *value)
{
	int i;

	for (i = 0; i < Ncontrol; i++){
		if (strcmp(name, controls[rec][i].name) == 0)
			break;
	}
	if (i == Ncontrol)
		return -1;
	if (controls[rec][i].readable == 0)
		return -1;
	if(getspecialcontrol(&controls[rec][i], rec, i, GET_CUR, value) < 0)
		return -1;
	memmove(controls[rec][i].value, value, sizeof controls[rec][i].value);
	return 0;
}

void
getcontrols(void)
{
	int rec, ctl;
	Audiocontrol *c;
	long v[9];

	for (rec = 0; rec < 2; rec++)
		for (ctl = 0; ctl < Ncontrol; ctl++){
			c = &controls[rec][ctl];
			if (c->readable){
				if (verbose)
					fprint(2, "%s %s control\n",
						rec?"Record":"Playback", controls[rec][ctl].name);
				getspecialcontrol(c, rec, ctl, GET_MIN, v);
				c->min = v[0];
				if (rec==Play && ctl ==Volume_control) 
					if (masterplayvol.min >= c->min) masterplayvol.min = c->min;
				if (rec==Record && ctl ==Volume_control) 
					if (masterrecvol.min >= c->min) masterrecvol.min = c->min;

				getspecialcontrol(c, rec, ctl, GET_MAX, v);
				c->max = v[0];
				if (rec==Play && ctl ==Volume_control)
					if (masterplayvol.max <= c->max) masterplayvol.max = c->max;
				if (rec==Record && ctl ==Volume_control) 
					if (masterrecvol.max <= c->max) masterrecvol.max = c->max;
				getspecialcontrol(c, rec, ctl, GET_RES, v);
				c->step = v[0];
				if (rec==Play && ctl ==Volume_control)
					masterplayvol.step = c->step;
				else if (rec==Record && ctl ==Volume_control)
					masterrecvol.step = c->step;
				getspecialcontrol(c, rec, ctl, GET_CUR, c->value);
				if (rec==Play && ctl ==Volume_control)
					masterplayvol.value = c->value[0];
				if (rec==Record && ctl ==Volume_control)
					masterrecvol.value = c->value[0];
			} else {
				c->min = Undef;
				c->max = Undef;
				c->step = Undef;
				c->value[0] = Undef;
				if (debug & Dbgcontrol)
					fprint(2, "%s %s control not readable\n",
						rec?"Playback":"Record", controls[rec][ctl].name);
			}
		}
}

int
ctlparse(char *s, Audiocontrol *c, long *v)
{
	int i, j, nf, m;
	char *vals[9];
	char *p;
	long val;

	nf = tokenize(s, vals, nelem(vals));
	if (nf <= 0)
		return -1;
	if (c->chans){
		j = 0;
		m = 0;
		SET(val);
		v[0] = 0;	// will compute average of v[i]
		for (i = 1; i < 9; i++)		/* K.Okamoto */
			if (c->chans & 1 << (i-1)) {	/* K.Okamoto */
				if (j < nf){
					val = strtol(vals[j], &p, 0);
					if (val == 0 && *p != '\0' && *p != '%')
						return -1;
					if (*p == '%' && c->min != Undef)
						val = (val*c->max + (100-val)*c->min)/100;
					j++;
				}
				v[i] = val;
				v[0] += val;
				m++;
			} else
				v[i] = Undef;
		if (m) v[0] /= m;
	} else {
		val = strtol(vals[0], &p, 0);
		if (*p == '%' && c->min != Undef)
			val = (val*c->max + (100-val)*c->min)/100;
		v[0] = val;
	}
	return 0;
}

int
Aconv(Fmt *fp)
{
	char str[256];
	Audiocontrol *c;
	int fst, i;
	char *p;

	c = va_arg(fp->args, Audiocontrol*);
	p = str;
	if (c->chans) {
		fst = 1;
		for (i = 1; i < 9; i++)		/* K.Okamoto */
			if (c->chans & 1 << (i-1)){		/* K.Okamoto */
				p = seprint(p, str+sizeof str, "%s%ld", fst?"'":" ", c->value[i]);
				fst = 0;
			}
		seprint(p, str+sizeof str, "'");
	} else
		seprint(p, str+sizeof str, "%ld", c->value[0]);
	return fmtstrcpy(fp, str);
}

/* added setselector command by K.Okamoto */
int
setselector(int id)
{
	int n;
	byte buf[2];

	buf[0] = id;		/* Select this id's feature for USB Streaming Output */
	if (setupcmd(ad->ep[0], RH2D|Rclass|Rinterface, SET_CUR, 0, selector<<8, buf, 1) < 0)
		return Undef;
	if (setupreq(ad->ep[0], RD2H|Rclass|Rinterface, GET_CUR, 0, selector<<8, 1) < 0)
		return Undef;
	n = setupreply(ad->ep[0], buf, 1);
	if (n == 1)
		fprint(2, "Selector Select Feature device = %d\n", buf[0]);
	return 0;
}

/* set all the controls permanent to mix, voice level = 0dB,  K.Okamoto */
int
initmixer()
{
	int m, n;
	byte buf[2];

	if(debug & Dbgcontrol) {
	if (verbose) fprint(2, "Getting default parameters of the MIXER\n");
	if (verbose) fprint(2, "GET_MAX\n");
	for (m = 1; m< 0x81; m=m*2)
		for (n=1; n<0x81; n=n*2)
			if (n == m) {
				if (setupreq(ad->ep[0], RD2H|Rclass|Rinterface, GET_MAX, n|m<<8, mixerid<<8, 2) < 0)
					fprint(2, "failed to get the value of input #m %d, output #n %d\n", m, n);;
				setupreply(ad->ep[0], buf, 2);
			}
	if (verbose) fprint(2, "GET_MIN\n");
	for (m = 1; m< 0x81; m=m*2)
		for (n=1; n<0x81; n=n*2)
			if (n == m) {
				if (setupreq(ad->ep[0], RD2H|Rclass|Rinterface, GET_MIN, n|m<<8, mixerid<<8, 2) < 0)
					fprint(2, "failed to get the value of input #m %d, output #n %d\n", m, n);;
				setupreply(ad->ep[0], buf, 2);
			}
	if (verbose) fprint(2, "GET_RES\n");
	for (m = 1; m< 0x81; m=m*2)
		for (n=1; n<0x81; n=n*2)
			if (n == m) {
				if (setupreq(ad->ep[0], RD2H|Rclass|Rinterface, GET_RES, n|m<<8, mixerid<<8, 2) < 0)
					fprint(2, "failed to get the value of input #m %d, output #n %d\n", m, n);;
				setupreply(ad->ep[0], buf, 2);
			}
	if (verbose) fprint(2, "GET_CUR\n");
	for (m = 1; m< 0x81; m=m*2)
		for (n=1; n<0x81; n=n*2)
			if (n == m) {
				if (setupreq(ad->ep[0], RD2H|Rclass|Rinterface, GET_CUR, n|m<<8, mixerid<<8, 2) < 0)
					fprint(2, "failed to get the value of input #m %d, output #n %d\n", m, n);;
				setupreply(ad->ep[0], buf, 2);
			}
	}

	/* program to set the endpoint to no Adaptive but Synchrounous K.Okamoto */
//(1) Left Front
	buf[0] = 0;
	buf[1] = 0;
	if (setupcmd(ad->ep[0], RH2D|Rclass|Rinterface, SET_CUR, 0x0101, mixerid<<8, buf, 2) < 0)
		return Undef;
	if (setupreq(ad->ep[0], RD2H|Rclass|Rinterface, GET_CUR, 0x0101, mixerid<<8, 2) < 0)
		return Undef;
	n = setupreply(ad->ep[0], buf, 2);
	if (n == 2)
		if (buf[0]|buf[1]<<8 == 0x8000)
			fprint(2, "Left Front = -oo\n");
		else
			fprint(2, "Left Front  = %ddB\n", buf[0]|buf[1]<<8);
//(2) Right Front
	buf[0] = 0;
	buf[1] = 0;
	if (setupcmd(ad->ep[0], RH2D|Rclass|Rinterface, SET_CUR, 0x0202, mixerid<<8, buf, 2) < 0)
		return Undef;
	if (setupreq(ad->ep[0], RD2H|Rclass|Rinterface, GET_CUR, 0x0202, mixerid<<8, 2) < 0)
		return Undef;
	n = setupreply(ad->ep[0], buf, 2);
	if (n == 2)
		if (buf[0]|buf[1]<<8 == 0x8000)
			fprint(2, "Right Front = -oo\n");
		else
			fprint(2, "Right Front  = %ddB\n", (buf[0]|buf[1]<<8));
/* belows are useless for the present CM-106/F chip K.Okamoto */
//(3) Center Front
	buf[0] = 0;
	buf[1] = 0;
	if (setupcmd(ad->ep[0], RH2D|Rclass|Rinterface, SET_CUR, 0x0404, mixerid<<8, buf, 2) < 0)
		return Undef;
	if (setupreq(ad->ep[0], RD2H|Rclass|Rinterface, GET_CUR, 0x0404, mixerid<<8, 2) < 0)
		return Undef;
	n = setupreply(ad->ep[0], buf, 2);
	if (n == 2)
		if (buf[0]|buf[1]<<8 == 0x8000)
			fprint(2, "Center Front = -oo\n");
		else
			fprint(2, "Center Front  = %ddB\n", buf[0]|buf[1]<<8);
//(4) SuperWoofer
	buf[0] = 0;
	buf[1] = 0;
	if (setupcmd(ad->ep[0], RH2D|Rclass|Rinterface, SET_CUR, 0x0808, mixerid<<8, buf, 2) < 0)
		return Undef;
	if (setupreq(ad->ep[0], RD2H|Rclass|Rinterface, GET_CUR, 0x0808, mixerid<<8, 2) < 0)
		return Undef;
	n = setupreply(ad->ep[0], buf, 2);
	if (n == 2)
		if (buf[0]|buf[1]<<8 == 0x8000)
			fprint(2, "Super Woofer = -oo\n");
		else
			fprint(2, "Super Woofer  = %ddB\n", buf[0]|buf[1]<<8);
//(5) Left Surround
	buf[0] = 0;
	buf[1] = 0;
	if (setupcmd(ad->ep[0], RH2D|Rclass|Rinterface, SET_CUR, 0x1010, mixerid<<8, buf, 2) < 0)
		return Undef;
	if (setupreq(ad->ep[0], RD2H|Rclass|Rinterface, GET_CUR, 0x1010, mixerid<<8, 2) < 0)
		return Undef;
	n = setupreply(ad->ep[0], buf, 2);
	if (n == 2)
		if (buf[0]|buf[1]<<8 == 0x8000)
			fprint(2, "Left Surround = -oo\n");
		else
			fprint(2, "Left Surround  = %ddB\n", buf[0]|buf[1]<<8);
//(6) Right Surround
	buf[0] = 0;
	buf[1] = 0;
	if (setupcmd(ad->ep[0], RH2D|Rclass|Rinterface, SET_CUR, 0x2020, mixerid<<8, buf, 2) < 0)
		return Undef;
	if (setupreq(ad->ep[0], RD2H|Rclass|Rinterface, GET_CUR, 0x2020, mixerid<<8, 2) < 0)
		return Undef;
	n = setupreply(ad->ep[0], buf, 2);
	if (n == 2)
		if (buf[0]|buf[1]<<8 == 0x8000)
			fprint(2, "Right Surround = -oo\n");
		else
			fprint(2, "Right Surround  = %ddB\n", buf[0]|buf[1]<<8);
	return 0;
}
