#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbproto.h"
#include "dat.h"
#include "fns.h"
#include "audioclass.h"

int buttonendpt =	-1;
Nexus *nexus[2];

char *controlname[Ncontrol] =
{
	[Speed_control]	"speed",
	[Mute_control]		"mute",
	[Volume_control]	"volume",
	[Bass_control]		"bass",
	[Mid_control]		"mid",
	[Treble_control]	"treble",
	[Equalizer_control]	"equalizer",
	[Agc_control]		"agc",
	[Delay_control]	"delay",
	[Bassboost_control]	"bassboost",
	[Loudness_control]	"loudness",
	[Channel_control]	"channels",
	[Resolution_control]	"resolution",
};

Nexus *
findnexus(char *name)
{
	if(strcmp(name, "playback") == 0)
		return nexus[Play];
	else if(strcmp(name, "record") == 0)
		return nexus[Record];
	else
		return nil;
}

int
controlnum(char *name)
{
	int i;

	for(i = 0; i < Ncontrol; i++)
		if(strcmp(controlname[i], name) == 0)
			return i;
	return Undef;
}

Audiocontrol *
findcontrol(Nexus *nx, char *name)
{
	int i;

	i = controlnum(name);
	if(i == Undef)
		return nil;
	return &nx->control[i];
}

static void
resetcontrol(Nexus *nx, int i)
{
	Audiocontrol *c;

	c = &nx->control[i];
	c->min = 1000000;
	c->max = 0;
	c->step = Undef;
}

static void
controlval(Nexus *nx, int i, int val)
{
	Audiocontrol *c;

	c = &nx->control[i];
	c->value[0] = val;
	if(val < c->min)
		c->min = val;
	if(val > c->max)
		c->max = val;
	c->settable = (c->min < c->max);
	c->readable = 1;
}

void
calcbounds(Nexus *nx)
{
	int i;
	Stream *s;
	Streamalt *sa;

	s = nx->s;
	if(s == nil)
		return;
	resetcontrol(nx, Channel_control);
	resetcontrol(nx, Resolution_control);
	resetcontrol(nx, Speed_control);
	for(sa = s->salt; sa != nil; sa = sa->next) {
		if(sa->nchan == 0)
			continue;
		controlval(nx, Channel_control, sa->nchan);
		controlval(nx, Resolution_control, sa->res);
		if(sa->nf == 0) {
			controlval(nx, Speed_control, sa->minf);
			controlval(nx, Speed_control, sa->maxf);
		}
		else {
			for(i = 0; i < sa->nf; i++)
				controlval(nx, Speed_control, sa->f[i]);
		}
	}
}

static Streamalt *
findalt(Nexus *nx, int nchan, int res, int speed, int *newspeed, int *newi)
{
	Stream *s;
	Streamalt *sa, *bestalt;
	int i, guess, err, besti, bestguess, besterr;

	s = nx->s;
	if(s == nil)
		return nil;
	bestalt = nil;
	besti = -1;
	bestguess = 0;
	besterr = 0x7fffffff;
	for(sa = s->salt; sa != nil; sa = sa->next) {
		if(sa->nchan != nchan || sa->res != res)
			continue;
		if(sa->nf == 0) {
			if(speed < sa->minf)
				guess = sa->minf;
			else if(speed > sa->maxf)
				guess = sa->maxf;
			else
				guess = speed;
			err = abs(speed - guess);
			if(err < besterr) {
				bestguess = guess;
				besti = -1;
				bestalt = sa;
				besterr = err;
			}
		}
		else {
			for(i = 0; i < sa->nf; i++) {
				err = abs(speed - sa->f[i]);
				if(err < besterr) {
					bestguess = sa->f[i];
					besti = i;
					bestalt = sa;
					besterr = err;
				}
			}
		}
	}
	*newspeed = bestguess;
	*newi = besti;
	return bestalt;
}

static int
altctl(Nexus *nx, int ctl, int val)
{
	Endpt *ep;
	Dalt *dalt;
	Device *d;
	uchar buf[3];
	Streamalt *sa;
	char cmdbuf[32];
	int ps, speed, newspeed, ind, nchan, res;

	nchan = nx->control[Channel_control].value[0];
	res = nx->control[Resolution_control].value[0];
	speed = nx->control[Speed_control].value[0];
	switch(ctl) {
	case Channel_control:
		nchan = val;
		break;
	case Resolution_control:
		res = val;
		break;
	case Speed_control:
		speed = val;
		break;
	}
	sa = findalt(nx, nchan, res, speed, &newspeed, &ind);
	if(sa == nil) {
		fprint(2, "cannot find alt: nchan %d res %d speed %d\n", nchan, res, speed);
		return Undef;
	}
	dalt = sa->dalt;
	if(dalt->npt == 0) {
		fprint(2, "no endpoint!\n");
		return Undef;
	}
	d = dalt->intf->d;
	if(setupreq(d, RH2D, SET_INTERFACE, dalt->alt, dalt->intf->x, nil, 0) < 0)
		return -1;
	ep = &dalt->ep[0];
	if(sa->attr & Asampfreq) {
		if(debug & Dbgcontrol)
			fprint(2, "Setting speed to %d Hz;", speed);
		if(ind >= 0) {
			buf[0] = ind;
			buf[1] = 0;
			buf[2] = 0;
		}
		else {
			buf[0] = newspeed;
			buf[1] = newspeed >> 8;
			buf[2] = newspeed >> 16;
		}
		if(setupreq(d, RH2D|Rclass|Rendpt, SET_CUR, sampling_freq_control<<8, ep->addr, buf, 3) < 0)
			return Undef;
		if(setupreq(d, RD2H|Rclass|Rendpt, GET_CUR, sampling_freq_control<<8, ep->addr, buf, 3) != 3)
			return Undef;
		if(buf[2]) {
			if (debug & Dbgcontrol)
				fprint(2, "Speed out of bounds %d (0x%x)\n",
					buf[0] | buf[1] << 8 | buf[2] << 16,
					buf[0] | buf[1] << 8 | buf[2] << 16);
		}
		else
			speed = buf[0] | buf[1] << 8 | buf[2] << 16;
		if (debug & Dbgcontrol)
			fprint(2, " speed now %d Hz;", speed);
	}
	else
		speed = newspeed;
	ps = ((speed * ep->pollms + 999) / 1000) * nchan * res/8;
	if(ps > ep->maxpkt){
		fprint(2, "packet size %d > maximum packet size %d", ps, ep->maxpkt);
		return Undef;
	}
	if(debug & Dbgcontrol)
		fprint(2, "Configuring %s endpoint for %d Hz\n", nx->name, speed);
	sprint(cmdbuf, "ep %d %d %c %d %d", ep->addr, ep->pollms, ep->dir == Ein ? 'r' : 'w', 
		nchan*res/8, speed);
	if(write(d->ctl, cmdbuf, strlen(cmdbuf)) != strlen(cmdbuf)){
		fprint(2, "writing %s to #U/usb%d/%d/ctl: %r\n", cmdbuf, d->ctlrno, d->id);
		return Undef;
	}
	if (debug & Dbgcontrol) fprint(2, "sent `%s' to /dev/usb%d/%d/ctl\n", cmdbuf, d->ctlrno, d->id);
	if(ctl == Speed_control)
		return speed;
	return val;
}

static int
set1(Nexus *nx, int ctl, int req, int i, int val)
{
	Device *d;
	byte buf[2];
	int type, count, id;

	if(nx->feat == nil || nx->s == nil)
		return Undef;
	d = nx->s->intf->d;
	id = nx->feat->id<<8;
	type = RH2D|Rclass|Rinterface;
	switch(ctl) {
	default:
		count = 1;
		break;
	case Volume_control:
	case Delay_control:
		count = 2;
		break;
	case Speed_control:
	case Channel_control:
	case Resolution_control:
		return Undef;
	}
	buf[0] = val;
	buf[1] = val>>8;
	if(setupreq(d, type, req, (ctl<<8) | i, id, buf, count) < 0)
		return Undef;
	return 0;
}

int
setcontrol(Nexus *nx, char *name, long *value)
{
	int i, ctl, m, req;
	Audiocontrol *c;

	ctl = controlnum(name);
	if (ctl == Undef){
		if (debug & Dbgcontrol) fprint(2, "setcontrol: control not found\n");
		return -1;
	}
	c = &nx->control[ctl];
	if(c->settable == 0) {
		if(debug & Dbgcontrol)
			fprint(2, "setcontrol: control %s.%d not settable\n", nx->name, ctl);
		if(c->chans) {
			for(i = 0; i < 8; i++)
				if((c->chans & 1 << i) && c->value[i] != value[i])
					return -1;
			return 0;
		}
		if(c->value[0] != value[0])
			return -1;
		return 0;
	}
	if(c->chans) {
		value[0] = 0;	// set to average
		m = 0;
		for(i = 1; i < 8; i++)
			if(c->chans & (1 << i)) {
				if(c->min != Undef && value[i] < c->min)
					value[i] = c->min;
				if(c->max != Undef && value[i] > c->max)
					value[i] = c->max;
				value[0] += value[i];
				m++;
			}
			else
				value[i] = Undef;
		if(m)
			value[0] /= m;
	}
	else {
		if(c->min != Undef && value[0] < c->min)
			value[0] = c->min;
		if(c->max != Undef && value[0] > c->max)
			value[0] = c->max;
	}
	req = SET_CUR;
	switch(ctl) {
	default:
		if(debug & Dbgcontrol)
			fprint(2, "setcontrol: can't happen\n");
		return -1;
	case Speed_control:
	case Resolution_control:
	case Channel_control:
		if((value[0] = altctl(nx, ctl, value[0])) < 0)
			return -1;
		c->value[0] = value[0];
		return 0;
	case Equalizer_control:
		/* not implemented */
		return -1;
	case Volume_control:
	case Delay_control:
	case Mute_control:
	case Bass_control:
	case Mid_control:
	case Treble_control:
	case Agc_control:
	case Bassboost_control:
	case Loudness_control:
		break;
	}
	if(c->chans) {
		for(i = 1; i < 8; i++)
			if(c->chans & 1 << i) {
				if(set1(nx, ctl, req, i, value[i]) < 0) {
					if(debug & Dbgcontrol)
						fprint(2, "setcontrol: setupcmd %s failed\n",
							controlname[ctl]);
					return -1;
				}
				c->value[i] = value[i];
			}
	}
	else {
		if(set1(nx, ctl, req, 0, value[0]) < 0) {
			if(debug & Dbgcontrol)
				fprint(2, "setcontrol: setupcmd %s failed\n",
					controlname[ctl]);
			return -1;
		}
	}
	c->value[0] = value[0];
	return 0;
}

static int
get1(Nexus *nx, int ctl, int req, int i)
{
	Device *d;
	byte buf[2];
	int type, count, id;

	if(nx->feat == nil || nx->s == nil)
		return Undef;
	d = nx->s->intf->d;
	id = nx->feat->id<<8;
	type = RD2H|Rclass|Rinterface;
	switch(ctl) {
	default:
		count = 1;
		break;
	case Volume_control:
	case Delay_control:
		count = 2;
		break;
	case Speed_control:
	case Channel_control:
	case Resolution_control:
		return Undef;
	}
	if(setupreq(d, type, req, (ctl<<8) | i, id, buf, count) != count)
		return Undef;
	switch(count) {
	case 1:
		return buf[0];
	case 2:
		return (short) (buf[0] | (buf[1]<<8));
	}
}

static int
getspecialcontrol(Nexus *nx, int ctl, int req, long *value)
{
	int m, i, val;
	Audiocontrol *c;

	c = &nx->control[ctl];
	switch(ctl) {
	default:
		return Undef;
	case Speed_control:
	case Channel_control:
	case Resolution_control:
		if(req == GET_MIN)
			value[0] = c->min;
		else if(req == GET_MAX)
			value[0] = c->max;
		else if(req == GET_RES)
			value[0] = c->step;
		else if(req == GET_CUR)
			value[0] = c->value[0];
		else
			value[0] = Undef;
		return 0;
	case Volume_control:
	case Delay_control:
	case Mute_control:
	case Bass_control:
	case Mid_control:
	case Treble_control:
	case Equalizer_control:
	case Agc_control:
	case Bassboost_control:
	case Loudness_control:
		break;
	}
	if(c->chans) {
		m = 0;
		value[0] = 0; // set to average
		for (i = 1; i < 8; i++){
			value[i] = Undef;
			if(c->chans & (1 << i)) {
				val = get1(nx, ctl, req, i);
				if(val == Undef)
					return Undef;
				if(req == GET_CUR) {
					value[i] = val;
					value[0] += val;
					m++;
				}
				else
					value[0] = val;
			}
		}
		if(m != 0)
			value[0] /= m;
		return 0;
	}
	value[0] = Undef;
	val = get1(nx, ctl, req, 0);
	if(val == Undef)
		return Undef;
	value[0] = val;
	return 0;
}

int
getcontrol(Nexus *nx, char *name, long *value)
{
	int i;
	Audiocontrol *c;

	i = controlnum(name);
	if(i == Undef)
		return -1;
	c = &nx->control[i];
	if(!c->readable)
		return -1;
	if(getspecialcontrol(nx, i, GET_CUR, value) < 0)
		return -1;
	memmove(c->value, value, sizeof c->value);
	return 0;
}

void
getcontrols(Nexus *nx)
{
	int ctl, i;
	Audiocontrol *c;
	long v[8];

	for(ctl = 0; ctl < Ncontrol; ctl++) {
		c = &nx->control[ctl];
		if(c->readable) {
			if(verbose)
				fprint(2, "%s %s control", nx->name, controlname[ctl]);
			c->min = (getspecialcontrol(nx, ctl, GET_MIN, v) < 0) ? Undef : v[0];
			if(verbose && c->min != Undef)
				fprint(2, ", min %ld", c->min);
			c->max = (getspecialcontrol(nx, ctl, GET_MAX, v) < 0) ? Undef : v[0];
			if(verbose && c->max != Undef)
				fprint(2, ", max %ld", c->max);
			c->step = (getspecialcontrol(nx, ctl, GET_RES, v) < 0) ? Undef : v[0];
			if(verbose && c->step != Undef)
				fprint(2, ", step %ld", c->step);
			if(getspecialcontrol(nx, ctl, GET_CUR, c->value) == 0) {
				if(verbose) {
					if(c->chans) {
						fprint(2, ", values");
						for(i = 1; i < 8; i++)
							if(c->chans & (1 << i))
								fprint(2, "[%d] %ld  ", i, c->value[i]);
					}
					else
						fprint(2, ", value %ld", c->value[0]);
				}
			}
			if(verbose)
				fprint(2, "\n");
		}
		else {
			c->min = Undef;
			c->max = Undef;
			c->step = Undef;
			c->value[0] = Undef;
			if (debug & Dbgcontrol)
				fprint(2, "%s %s control not settable\n", nx->name, controlname[ctl]);
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
