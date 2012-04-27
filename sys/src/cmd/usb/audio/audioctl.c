#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "audio.h"
#include "audioctl.h"

int endpt[2] =		{-1, -1};
int interface[2] =	{-1, -1};
int featureid[2] =	{-1, -1};
int selectorid[2] =	{-1, -1};
int mixerid[2] =	{-1, -1};
int curalt[2] =		{-1, -1};
int buttonendpt =	-1;

int id;
Dev *ad;

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
	dprint(2, "setcontrol %s: Set alt %d\n", c->name, control);
	curalt[rec] = control;
	if(usbcmd(ad, Rh2d|Rstd|Riface, Rsetiface, control, interface[rec], nil, 0) < 0){
		dprint(2, "setcontrol: setupcmd %s failed\n", c->name);
		return -1;
	}
	return control;
}

int
findalt(int rec, int nchan, int res, int speed)
{
	Ep *ep;
	Audioalt *a;
	Altc *da;
	int i, j, k, retval;

	retval = -1;
	controls[rec][Channel_control].min = 1000000;
	controls[rec][Channel_control].max = 0;
	controls[rec][Channel_control].step = Undef;
	controls[rec][Resolution_control].min = 1000000;
	controls[rec][Resolution_control].max = 0;
	controls[rec][Resolution_control].step = Undef;
	for(i = 0; i < nelem(ad->usb->ep); i++){
		if((ep = ad->usb->ep[i]) == nil)
			continue;
		if(ep->iface == nil){
			fprint(2, "\tno interface\n");
			return 0;
		}
		if(ep->iface->csp != CSP(Claudio, 2, 0))
			continue;
		if((rec == Play && (ep->addr &  0x80))
		|| (rec == Record && (ep->addr &  0x80) == 0))
			continue;
		for(j = 0; j < 16; j++){
			if((da = ep->iface->altc[j]) == nil || (a = da->aux) == nil)
				continue;
			if(a->nchan < controls[rec][Channel_control].min)
				controls[rec][Channel_control].min = a->nchan;
			if(a->nchan > controls[rec][Channel_control].max)
				controls[rec][Channel_control].max = a->nchan;
			if(a->res < controls[rec][Resolution_control].min)
				controls[rec][Resolution_control].min = a->res;
			if(a->res > controls[rec][Resolution_control].max)
				controls[rec][Resolution_control].max = a->res;
			controls[rec][Channel_control].settable = 1;
			controls[rec][Channel_control].readable = 1;
			controls[rec][Resolution_control].settable = 1;
			controls[rec][Resolution_control].readable = 1;
			controls[rec][Speed_control].settable = 1;
			controls[rec][Speed_control].readable = 1;
			if(a->nchan == nchan && a->res == res){
				if(speed == Undef)
					retval = j;
				else if(a->caps & (has_discfreq|onefreq)){
					for(k = 0; k < nelem(a->freqs); k++){
						if(a->freqs[k] == speed){
							retval = j;
							break;
						}
					}
				}else{
					if(speed >= a->minfreq && speed <= a->maxfreq)
						retval = j;
				}
			}
		}
	}
	if(usbdebug && retval < 0)
		fprint(2, "findalt(%d, %d, %d, %d) failed\n", rec, nchan, res, speed);
	return retval;
}

int
setspeed(int rec, int speed)
{
	int ps, n, no, dist, i;
	Audioalt *a;
	Altc *da;
	Ep *ep;
	uchar buf[3];

	if(rec == Record && !setrec)
		return Undef;
	if(curalt[rec] < 0){
		fprint(2, "Must set channels and resolution before speed\n");
		return Undef;
	}
	if(endpt[rec] < 0)
		sysfatal("endpt[%s] not set", rec?"Record":"Playback");
	ep = ad->usb->ep[endpt[rec]];
	if(ep->iface == nil)
		sysfatal("no interface");
	if(curalt[rec] < 0)
		sysfatal("curalt[%s] not set", rec?"Record":"Playback");
	da = ep->iface->altc[curalt[rec]];
	a = da->aux;
	if(a->caps & onefreq){
		dprint(2, "setspeed %d: onefreq\n", speed);
		/* speed not settable, but packet size must still be set */
		speed = a->freqs[0];
	}else if(a->caps & has_contfreq){
		dprint(2, "setspeed %d: contfreq\n", speed);
		if(speed < a->minfreq)
			speed = a->minfreq;
		else if(speed > a->maxfreq)
			speed = a->maxfreq;
		dprint(2, "Setting continuously variable %s speed to %d\n",
				rec?"record":"playback", speed);
	}else if(a->caps & has_discfreq){
		dprint(2, "setspeed %d: discfreq\n", speed);
		dist = 1000000;
		no = -1;
		for(i = 0; a->freqs[i] > 0; i++)
			if(abs(a->freqs[i] - speed) < dist){
				dist = abs(a->freqs[i] - speed);
				no = i;
			}
		if(no == -1){
			dprint(2, "no = -1\n");
			return Undef;
		}
		speed = a->freqs[no];
		dprint(2, "Setting discreetly variable %s speed to %d\n",
				rec?"record":"playback", speed);
	}else{
		dprint(2, "can't happen\n?");
		return Undef;
	}
	if(a->caps & has_setspeed){
		dprint(2, "Setting %s speed to %d Hz;", rec?"record":"playback", speed);
		buf[0] = speed;
		buf[1] = speed >> 8;
		buf[2] = speed >> 16;
		n = endpt[rec];
		if(rec)
			n |= 0x80;
		if(usbcmd(ad, Rh2d|Rclass|Rep, Rsetcur, sampling_freq_control<<8, n, buf, 3) < 0){
			fprint(2, "Error in setupcmd\n");
			return Undef;
		}
		if((n=usbcmd(ad, Rd2h|Rclass|Rep, Rgetcur, sampling_freq_control<<8, n, buf, 3)) < 0){
			fprint(2, "Error in setupreq\n");
			return Undef;
		}
		if(n != 3)
			fprint(2, "Error in setupreply: %d\n", n);
		else{
			n = buf[0] | buf[1] << 8 | buf[2] << 16;
			if(buf[2] || n == 0){
				dprint(2, "Speed out of bounds %d (0x%x)\n", n, n);
			}else if(n != speed && ad->usb->vid == 0x077d &&
			    (ad->usb->did == 0x0223 || ad->usb->did == 0x07af)){
				/* Griffin iMic responds incorrectly to sample rate inquiry */
				dprint(2, " reported as %d (iMic bug?);", n);
			}else
				speed = n;
		}
		dprint(2, " speed now %d Hz;", speed);
	}
	ps = ((speed * da->interval + 999) / 1000)
		* controls[rec][Channel_control].value[0]
		* controls[rec][Resolution_control].value[0]/8;
	if(ps > ep->maxpkt){
		fprint(2, "%s: setspeed(rec %d, speed %d): packet size %d > "
			"maximum packet size %d\n",
			argv0, rec, speed, ps, ep->maxpkt);
		return Undef;
	}
	dprint(2, "Configuring %s endpoint for %d Hz\n",
				rec?"record":"playback", speed);
	epdev[rec] = openep(ad, endpt[rec]);
	if(epdev[rec] == nil)
		sysfatal("openep rec %d: %r", rec);

	devctl(epdev[rec], "pollival %d", da->interval);
	devctl(epdev[rec], "samplesz %ld", controls[rec][Channel_control].value[0] *
				controls[rec][Resolution_control].value[0]/8);
	devctl(epdev[rec], "hz %d", speed);

	/* NO: the client uses the endpoint file directly
	if(opendevdata(epdev[rec], rec ? OREAD : OWRITE) < 0)
		sysfatal("openep rec %d: %r", rec);
	*/
	return speed;
}

long
getspeed(int rec, int which)
{
	int i, n;
	Audioalt *a;
	Altc *da;
	Ep *ep;
	uchar buf[3];
	int r;

	if(curalt[rec] < 0){
		fprint(2, "Must set channels and resolution before getspeed\n");
		return Undef;
	}
	if(endpt[rec] < 0)
		sysfatal("endpt[%s] not set", rec?"Record":"Playback");
	dprint(2, "getspeed: endpt[%d] == %d\n", rec, endpt[rec]);
	ep = ad->usb->ep[endpt[rec]];
	if(ep->iface == nil)
		sysfatal("no interface");
	if(curalt[rec] < 0)
		sysfatal("curalt[%s] not set", rec?"Record":"Playback");
	da = ep->iface->altc[curalt[rec]];
	a = da->aux;
	if(a->caps & onefreq){
		dprint(2, "getspeed: onefreq\n");
		if(which == Rgetres)
			return Undef;
		return a->freqs[0];		/* speed not settable */
	}
	if(a->caps & has_setspeed){
		dprint(2, "getspeed: has_setspeed, ask\n");
		n = endpt[rec];
		if(rec)
			n |= 0x80;
		r = Rd2h|Rclass|Rep;
		if(usbcmd(ad,r,which,sampling_freq_control<<8, n, buf, 3) < 0)
			return Undef;
		if(n == 3){
			if(buf[2]){
				dprint(2, "Speed out of bounds\n");
				if((a->caps & has_discfreq) && (buf[0] | buf[1] << 8) < 8)
					return a->freqs[buf[0] | buf[1] << 8];
			}
			return buf[0] | buf[1] << 8 | buf[2] << 16;
		}
		dprint(2, "getspeed: n = %d\n", n);
	}
	if(a->caps & has_contfreq){
		dprint(2, "getspeed: has_contfreq\n");
		if(which == Rgetcur)
			return controls[rec][Speed_control].value[0];
		if(which == Rgetmin)
			return a->minfreq;
		if(which == Rgetmax)
			return a->maxfreq;
		if(which == Rgetres)
			return 1;
	}
	if(a->caps & has_discfreq){
		dprint(2, "getspeed: has_discfreq\n");
		if(which == Rgetcur)
			return controls[rec][Speed_control].value[0];
		if(which == Rgetmin)
			return a->freqs[0];
		for(i = 0; i < 8 && a->freqs[i] > 0; i++)
			;
		if(which == Rgetmax)
			return a->freqs[i-1];
		if(which == Rgetres)
			return Undef;
	}
	dprint(2, "can't happen\n?");
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
	for(ctl = 0; ctl < Ncontrol; ctl++){
		c = &controls[rec][ctl];
		if(strcmp(name, c->name) == 0)
			break;
	}
	if(ctl == Ncontrol){
		dprint(2, "setcontrol: control not found\n");
		return -1;
	}
	if(c->settable == 0){
		dprint(2, "setcontrol: control %d.%d not settable\n", rec, ctl);
		if(c->chans){
			for(i = 0; i < 8; i++)
				if((c->chans & 1 << i) && c->value[i] != value[i])
					return -1;
			return 0;
		}
		if(c->value[0] != value[0])
			return -1;
		return 0;
	}
	if(c->chans){
		value[0] = 0;	// set to average
		m = 0;
		for(i = 1; i < 8; i++)
			if(c->chans & 1 << i){
				if(c->min != Undef && value[i] < c->min)
					value[i] = c->min;
				if(c->max != Undef && value[i] > c->max)
					value[i] = c->max;
				value[0] += value[i];
				m++;
			}else
				value[i] = Undef;
		if(m) value[0] /= m;
	}else{
		if(c->min != Undef && value[0] < c->min)
			value[0] = c->min;
		if(c->max != Undef && value[0] > c->max)
			value[0] = c->max;
	}
	req = Rsetcur;
	count = 1;
	switch(ctl){
	default:
		dprint(2, "setcontrol: can't happen\n");
		return -1;
	case Speed_control:
		if((rec != Record || setrec) && (value[0] = setspeed(rec, value[0])) < 0)
			return -1;
		c->value[0] = value[0];
		return 0;
	case Equalizer_control:
		/* not implemented */
		return -1;
	case Resolution_control:
		control = findalt(rec, controls[rec][Channel_control].value[0], value[0], defaultspeed[rec]);
		if(control < 0 || setaudioalt(rec, c, control) < 0){
			dprint(2, "setcontrol: can't find setting for %s\n", c->name);
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
		type = Rh2d|Rclass|Riface;
		control = ctl<<8;
		index = featureid[rec]<<8;
		break;
	case Selector_control:
		type = Rh2d|Rclass|Riface;
		control = 0;
		index = selectorid[rec]<<8;
		break;
	case Channel_control:
		control = findalt(rec, value[0], controls[rec][Resolution_control].value[0], defaultspeed[rec]);
		if(control < 0 || setaudioalt(rec, c, control) < 0){
			dprint(2, "setcontrol: can't find setting for %s\n", c->name);
			return -1;
		}
		c->value[0] = value[0];
		controls[rec][Speed_control].value[0] = defaultspeed[rec];
		return 0;
	}
	if(c->chans){
		for(i = 1; i < 8; i++)
			if(c->chans & 1 << i){
				switch(count){
				case 2:
					buf[1] = value[i] >> 8;
				case 1:
					buf[0] = value[i];
				}
				if(usbcmd(ad, type, req, control | i, index, buf, count) < 0){
					dprint(2, "setcontrol: setupcmd %s failed\n",
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
		if(usbcmd(ad, type, req, control, index, buf, count) < 0){
			dprint(2, "setcontrol: setupcmd %s failed\n", c->name);
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
	int type, control, index, count, signedbyte;
	short svalue;

	count = 1;
	signedbyte = 0;
	switch(ctl){
	default:
		return Undef;
	case Speed_control:
		value[0] =  getspeed(rec, req);
		return 0;
	case Channel_control:
	case Resolution_control:
		if(req == Rgetmin)
			value[0] = controls[rec][ctl].min;
		if(req == Rgetmax)
			value[0] = controls[rec][ctl].max;
		if(req == Rgetres)
			value[0] = controls[rec][ctl].step;
		if(req == Rgetcur)
			value[0] = controls[rec][ctl].value[0];
		return 0;
	case Volume_control:
	case Delay_control:
		count = 2;
		/* fall through */
	case Bass_control:
	case Mid_control:
	case Treble_control:
	case Equalizer_control:
		signedbyte = 1;
		type = Rd2h|Rclass|Riface;
		control = ctl<<8;
		index = featureid[rec]<<8;
		break;
	case Selector_control:
		type = Rd2h|Rclass|Riface;
		control = 0;
		index = selectorid[rec]<<8;
		break;
	case Mute_control:
	case Agc_control:
	case Bassboost_control:
	case Loudness_control:
		if(req != Rgetcur)
			return Undef;
		type = Rd2h|Rclass|Riface;
		control = ctl<<8;
		index = featureid[rec]<<8;
		break;
	}
	if(controls[rec][ctl].chans){
		m = 0;
		value[0] = 0; // set to average
		for(i = 1; i < 8; i++){
			value[i] = Undef;
			if(controls[rec][ctl].chans & 1 << i){
				n=usbcmd(ad, type,req, control|i,index,buf,count);
				if(n < 0)
					return Undef;
				if(n != count)
					return -1;
				switch (count){
				case 2:
					svalue = buf[1] << 8 | buf[0];
					if(req == Rgetcur){
						value[i] = svalue;
						value[0] += svalue;
						m++;
					}else
						value[0] = svalue;
					break;
				case 1:
					svalue = buf[0];
					if(signedbyte && (svalue&0x80))
						svalue |= 0xFF00;
					if(req == Rgetcur){
						value[i] = svalue;
						value[0] += svalue;
						m++;
					}else
						value[0] = svalue;
				}
			}
		}
		if(m) value[0] /= m;
		return 0;
	}
	value[0] = Undef;
	if(usbcmd(ad, type, req, control, index, buf, count) != count)
		return -1;
	switch (count){
	case 2:
		svalue = buf[1] << 8 | buf[0];
		value[0] = svalue;
		break;
	case 1:
		svalue = buf[0];
		if(signedbyte && (svalue&0x80))
			svalue |= 0xFF00;
		value[0] = svalue;
	}
	return 0;
}

int
getcontrol(int rec, char *name, long *value)
{
	int i;

	for(i = 0; i < Ncontrol; i++){
		if(strcmp(name, controls[rec][i].name) == 0)
			break;
	}
	if(i == Ncontrol)
		return -1;
	if(controls[rec][i].readable == 0)
		return -1;
	if(getspecialcontrol(rec, i, Rgetcur, value) < 0)
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

	for(rec = 0; rec < 2; rec++){
		if(rec == Record && !setrec)
			continue;
		for(ctl = 0; ctl < Ncontrol; ctl++){
			c = &controls[rec][ctl];
			if(c->readable){
				if(verbose)
					fprint(2, "%s %s control",
						rec?"Record":"Playback", controls[rec][ctl].name);
				c->min = (getspecialcontrol(rec, ctl, Rgetmin, v) < 0) ? Undef : v[0];
				if(verbose && c->min != Undef)
					fprint(2, ", min %ld", c->min);
				c->max = (getspecialcontrol(rec, ctl, Rgetmax, v) < 0) ? Undef : v[0];
				if(verbose && c->max != Undef)
					fprint(2, ", max %ld", c->max);
				c->step = (getspecialcontrol(rec, ctl, Rgetres, v) < 0) ? Undef : v[0];
				if(verbose && c->step != Undef)
					fprint(2, ", step %ld", c->step);
				if(getspecialcontrol(rec, ctl, Rgetcur, c->value) == 0){
					if(verbose){
						if(c->chans){
							fprint(2, ", values");
							for(i = 1; i < 8; i++)
								if(c->chans & 1 << i)
									fprint(2, "[%d] %ld  ", i, c->value[i]);
						}else
							fprint(2, ", value %ld", c->value[0]);
					}
				}
				if(verbose)
					fprint(2, "\n");
			}else{
				c->min = Undef;
				c->max = Undef;
				c->step = Undef;
				c->value[0] = Undef;
				dprint(2, "%s %s control not settable\n",
					rec?"Playback":"Record", controls[rec][ctl].name);
			}
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
	if(nf <= 0)
		return -1;
	if(c->chans){
		j = 0;
		m = 0;
		SET(val);
		v[0] = 0;	// will compute average of v[i]
		for(i = 1; i < 8; i++)
			if(c->chans & 1 << i){
				if(j < nf){
					val = strtol(vals[j], &p, 0);
					if(val == 0 && *p != '\0' && *p != '%')
						return -1;
					if(*p == '%' && c->min != Undef)
						val = (val*c->max + (100-val)*c->min)/100;
					j++;
				}
				v[i] = val;
				v[0] += val;
				m++;
			}else
				v[i] = Undef;
		if(m) v[0] /= m;
	}else{
		val = strtol(vals[0], &p, 0);
		if(*p == '%' && c->min != Undef)
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
	if(c->chans){
		fst = 1;
		for(i = 1; i < 8; i++)
			if(c->chans & 1 << i){
				p = seprint(p, str+sizeof str, "%s%ld", fst?"'":" ", c->value[i]);
				fst = 0;
			}
		seprint(p, str+sizeof str, "'");
	}else
		seprint(p, str+sizeof str, "%ld", c->value[0]);
	return fmtstrcpy(fp, str);
}
