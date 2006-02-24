#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbaudio.h"
#include "usbaudioctl.h"

int endpt[2] =		{-1, -1};
int interface[2] =	{-1, -1};
int featureid[2] =	{-1, -1};
int selectorid[2] =	{-1, -1};
int mixerid[2] =	{-1, -1};
int curalt[2] =		{-1, -1};
int buttonendpt =	-1;

int id;
Device *ad;

Audiocontrol controls[2][Ncontrol] = {
	{
	[Speed_control] = {		"speed",	0, {0}, 0,	44100,	Undef},
	[Mute_control] = {		"mute",		0, {0}, 0,	0,	Undef},
	[Volume_control] = {		"volume",	0, {0}, 0,	0,	Undef},
	[Bass_control] = {		"bass",		0, {0}, 0,	0,	Undef},
	[Mid_control] = {		"mid",		0, {0}, 0,	0,	Undef},
	[Treble_control] = {		"treble",	0, {0}, 0,	0,	Undef},
	[Equalizer_control] = {		"equalizer",	0, {0}, 0,	0,	Undef},
	[Agc_control] = {		"agc",		0, {0}, 0,	0,	Undef},
	[Delay_control] = {		"delay",	0, {0}, 0,	0,	Undef},
	[Bassboost_control] = {		"bassboost",	0, {0}, 0,	0,	Undef},
	[Loudness_control] = {		"loudness",	0, {0}, 0,	0,	Undef},
	[Channel_control] = {		"channels",	0, {0}, 0,	2,	Undef},
	[Resolution_control] = {	"resolution",	0, {0}, 0,	16,	Undef},
//	[Selector_control] = {		"selector",	0, {0}, 0,	0,	Undef},
	}, {
	[Speed_control] = {		"speed",	0, {0}, 0,	44100,	Undef},
	[Mute_control] = {		"mute",		0, {0}, 0,	0,	Undef},
	[Volume_control] = {		"volume",	0, {0}, 0,	0,	Undef},
	[Bass_control] = {		"bass",		0, {0}, 0,	0,	Undef},
	[Mid_control] = {		"mid",		0, {0}, 0,	0,	Undef},
	[Treble_control] = {		"treble",	0, {0}, 0,	0,	Undef},
	[Equalizer_control] = {		"equalizer",	0, {0}, 0,	0,	Undef},
	[Agc_control] = {		"agc",		0, {0}, 0,	0,	Undef},
	[Delay_control] = {		"delay",	0, {0}, 0,	0,	Undef},
	[Bassboost_control] = {		"bassboost",	0, {0}, 0,	0,	Undef},
	[Loudness_control] = {		"loudness",	0, {0}, 0,	0,	Undef},
	[Channel_control] = {		"channels",	0, {0}, 0,	2,	Undef},
	[Resolution_control] = {	"resolution",	0, {0}, 0,	16,	Undef},
//	[Selector_control] = {		"selector",	0, {0}, 0,	0,	Undef},
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
findalt(int rec, int nchan, int res, int speed)
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
		if ((rec == Play && (ep->addr &  0x80))
		|| (rec == Record && (ep->addr &  0x80) == 0))
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
	if ((debug & Dbgcontrol) && retval < 0){
		fprint(2, "findalt(%d, %d, %d, %d) failed\n", rec, nchan, res, speed);
	}
	return retval;
}

int
setspeed(int rec, int speed)
{
	int ps, n, no, dist, i;
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
	if (a->caps & onefreq){
		if (debug & Dbgcontrol)
			fprint(2, "setspeed %d: onefreq\n", speed);
		speed = a->freqs[0];		/* speed not settable, but packet size must still be set */
	}else if (a->caps & has_contfreq){
		if (debug & Dbgcontrol)
			fprint(2, "setspeed %d: contfreq\n", speed);
		if (speed < a->minfreq)
			speed = a->minfreq;
		else if (speed > a->maxfreq)
			speed = a->maxfreq;
		if (debug & Dbgcontrol)
			fprint(2, "Setting continuously variable %s speed to %d\n",
				rec?"record":"playback", speed);
	}else if (a->caps & has_discfreq){
		if (debug & Dbgcontrol)
			fprint(2, "setspeed %d: discfreq\n", speed);
		dist = 1000000;
		no = -1;
		for (i = 0; a->freqs[i] > 0; i++)
			if (abs(a->freqs[i] - speed) < dist){
				dist = abs(a->freqs[i] - speed);
				no = i;
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
			fprint(2, "Setting %s speed to %d Hz;", rec?"record":"playback", speed);
		buf[0] = speed;
		buf[1] = speed >> 8;
		buf[2] = speed >> 16;
		if(setupcmd(ad->ep[0], RH2D|Rclass|Rendpt, SET_CUR, sampling_freq_control<<8, endpt[rec], buf, 3) < 0){
			fprint(2, "Error in setupcmd\n");
			return Undef;
		}
		if (setupreq(ad->ep[0], RD2H|Rclass|Rendpt, GET_CUR, sampling_freq_control<<8, endpt[rec], 3) < 0){
			fprint(2, "Error in setupreq\n");
			return Undef;
		}
		n = setupreply(ad->ep[0], buf, 3);
		if (n != 3)
			fprint(2, "Error in setupreply: %d\n", n);
		else if (buf[2]){
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
		fprint(2, "packet size %d > maximum packet size %d\n",
			ps, ep->maxpkt);
		return Undef;
	}
	if (debug & Dbgcontrol)
		fprint(2, "Configuring %s endpoint for %d Hz\n",
				rec?"record":"playback", speed);
	sprint(cmdbuf, "ep %d %d %c %ld %d", endpt[rec], da->interval, rec?'r':'w', 
		controls[rec][Channel_control].value[0]*controls[rec][Resolution_control].value[0]/8,
		speed);
	if (write(ad->ctl, cmdbuf, strlen(cmdbuf)) != strlen(cmdbuf)){
		fprint(2, "writing %s to #U/usb/%d/ctl: %r\n", cmdbuf, id);
		return Undef;
	}
	if (debug & Dbgcontrol) fprint(2, "sent `%s' to /dev/usb/%d/ctl\n", cmdbuf, id);
	return speed;
}

long
getspeed(int rec, int which)
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
	if(debug & Dbgcontrol)
		fprint(2, "getspeed: curalt[%d] == %d\n", rec, endpt[rec]);
	ep = ad->ep[endpt[rec]];
	if (ep->iface == nil)
		sysfatal("no interface");
	if (curalt[rec] < 0)
		sysfatal("curalt[%s] not set\n", rec?"Record":"Playback");
	da = ep->iface->dalt[curalt[rec]];
	a = da->devspec;
	if (a->caps & onefreq){
		if(debug & Dbgcontrol)
			fprint(2, "getspeed: onefreq\n");
		if (which == GET_RES)
			return Undef;
		return a->freqs[0];		/* speed not settable */
	}
	if (a->caps & has_setspeed){
		if(debug & Dbgcontrol)
			fprint(2, "getspeed: has_setspeed, ask\n");
		if (setupreq(ad->ep[0], RD2H|Rclass|Rendpt, which, sampling_freq_control<<8, endpt[rec], 3) < 0)
			return Undef;
		n = setupreply(ad->ep[0], buf, 3);
		if(n == 3){
			if(buf[2]){
				if (debug & Dbgcontrol)
					fprint(2, "Speed out of bounds\n");
				if ((a->caps & has_discfreq) && (buf[0] | buf[1] << 8) < 8)
					return a->freqs[buf[0] | buf[1] << 8];
			}
			return buf[0] | buf[1] << 8 | buf[2] << 16;
		}
		if(debug & Dbgcontrol)
			fprint(2, "getspeed: n = %d\n", n);
	}
	if (a->caps & has_contfreq){
		if(debug & Dbgcontrol)
			fprint(2, "getspeed: has_contfreq\n");
		if (which == GET_CUR)
			return controls[rec][Speed_control].value[0];
		if (which == GET_MIN)
			return a->minfreq;
		if (which == GET_MAX)
			return a->maxfreq;
		if (which == GET_RES)
			return 1;
	}
	if (a->caps & has_discfreq){
		if(debug & Dbgcontrol)
			fprint(2, "getspeed: has_discfreq\n");
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

int
setcontrol(int rec, char *name, long *value)
{
	int i, ctl, m;
	byte buf[3];
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
			for (i = 0; i < 8; i++)
				if ((c->chans & 1 << i) && c->value[i] != value[i])
					return -1;
			return 0;
		}
		if (c->value[0] != value[0])
			return -1;
		return 0;
	}
	if (c->chans){
		value[0] = 0;	// set to average
		m = 0;
		for (i = 1; i < 8; i++)
			if (c->chans & 1 << i){
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
	case Delay_control:
		count = 2;
		/* fall through */
	case Mute_control:
	case Bass_control:
	case Mid_control:
	case Treble_control:
	case Agc_control:
	case Bassboost_control:
	case Loudness_control:
		type = RH2D|Rclass|Rinterface;
		control = ctl<<8;
		index = featureid[rec]<<8;
		break;
	case Selector_control:
		type = RH2D|Rclass|Rinterface;
		control = ctl<<8;
		index = selectorid[rec];
		break;
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
	}
	if(c->chans){
		for (i = 1; i < 8; i++)
			if (c->chans & 1 << i){
				switch(count){
				case 2:
					buf[1] = value[i] >> 8;
				case 1:
					buf[0] = value[i];
				}
				if (setupcmd(ad->ep[0], type, req, control | i, index, buf, count) < 0){
					if (debug & Dbgcontrol) fprint(2, "setcontrol: setupcmd %s failed\n",
						controls[rec][ctl].name);
					return -1;
				}
				c->value[i] = value[i];
			}
	}else{
		switch(count){
		case 2:
			buf[1] = value[0] >> 8;
		case 1:
			buf[0] = value[0];
		}
		if (setupcmd(ad->ep[0], type, req, control, index, buf, count) < 0){
			if (debug & Dbgcontrol) fprint(2, "setcontrol: setupcmd %s failed\n",
				c->name);
			return -1;
		}
	}
	c->value[0] = value[0];
	return 0;
}

int
getspecialcontrol(int rec, int ctl, int req, long *value)
{
	byte buf[3];
	int m, n, i;
	int type, control, index, count;
	short svalue;

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
	case Delay_control:
		count = 2;
		/* fall through */
	case Mute_control:
	case Bass_control:
	case Mid_control:
	case Treble_control:
	case Equalizer_control:
	case Agc_control:
	case Bassboost_control:
	case Loudness_control:
		type = RD2H|Rclass|Rinterface;
		control = ctl<<8;
		index = featureid[rec]<<8;
		break;
	case Selector_control:
		type = RD2H|Rclass|Rinterface;
		control = ctl<<8;
		index = selectorid[rec];
		break;
	}
	if (controls[rec][ctl].chans){
		m = 0;
		value[0] = 0; // set to average
		for (i = 1; i < 8; i++){
			value[i] = Undef;
			if (controls[rec][ctl].chans & 1 << i){
				if (setupreq(ad->ep[0], type, req, control | i, index, count) < 0)
					return Undef;
				n = setupreply(ad->ep[0], buf, count);
				if (n != count)
					return -1;
				switch (count) {
				case 2:
					svalue = buf[1] << 8 | buf[0];
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
		if (m) value[0] /= m;
		return 0;
	}
	value[0] = Undef;
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
	if(getspecialcontrol(rec, i, GET_CUR, value) < 0)
		return -1;
	memmove(controls[rec][i].value, value, sizeof controls[rec][i].value);
	return 0;
}

void
getcontrols(void)
{
	int rec, ctl, i;
	Audiocontrol *c;
	long v[8];

	for (rec = 0; rec < 2; rec++)
		for (ctl = 0; ctl < Ncontrol; ctl++){
			c = &controls[rec][ctl];
			if (c->readable){
				if (verbose)
					fprint(2, "%s %s control",
						rec?"Record":"Playback", controls[rec][ctl].name);
				c->min = (getspecialcontrol(rec, ctl, GET_MIN, v) < 0) ? Undef : v[0];
				if (verbose && c->min != Undef)
					fprint(2, ", min %ld", c->min);
				c->max = (getspecialcontrol(rec, ctl, GET_MAX, v) < 0) ? Undef : v[0];
				if (verbose && c->max != Undef)
					fprint(2, ", max %ld", c->max);
				c->step = (getspecialcontrol(rec, ctl, GET_RES, v) < 0) ? Undef : v[0];
				if (verbose && c->step != Undef)
					fprint(2, ", step %ld", c->step);
				if (getspecialcontrol(rec, ctl, GET_CUR, c->value) == 0){
					if (verbose) {
						if (c->chans){
							fprint(2, ", values");
							for (i = 1; i < 8; i++)
								if (c->chans & 1 << i)
									fprint(2, "[%d] %ld  ", i, c->value[i]);
						}else
							fprint(2, ", value %ld", c->value[0]);
					}
				}
				if (verbose)
					fprint(2, "\n");
			} else {
				c->min = Undef;
				c->max = Undef;
				c->step = Undef;
				c->value[0] = Undef;
				if (debug & Dbgcontrol)
					fprint(2, "%s %s control not settable\n",
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
		for (i = 1; i < 8; i++)
			if (c->chans & 1 << i) {
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
		for (i = 1; i < 8; i++)
			if (c->chans & 1 << i){
				p = seprint(p, str+sizeof str, "%s%ld", fst?"'":" ", c->value[i]);
				fst = 0;
			}
		seprint(p, str+sizeof str, "'");
	} else
		seprint(p, str+sizeof str, "%ld", c->value[0]);
	return fmtstrcpy(fp, str);
}
