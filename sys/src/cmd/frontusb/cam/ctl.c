#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "usb.h"
#include "uvc.h"
#include "dat.h"
#include "fns.h"

typedef struct Param Param;

enum {
	PARAMSPEC,
	PARAMCT,
	PARAMPU,
};

struct Param {
	char *name;
	int type;
	int cs;
	int len;
	int flag;
	char *(*read)(Cam *, int, Param *);
	int (*write)(Cam *, int, Param *, char **, int);
	void *auxp;
	int auxi;
};

void
errorcode(Dev *d, int term)
{
	uchar val;
	char *str[] = {
		"No error",
		"Not ready",
		"Wrong state",
		"Power",
		"Out of range",
		"Invalid unit",
		"Invalid control",
		"Invalid Request",
		"Invalid value within range"
	};
	if(usbcmd(d, 0xA1, GET_CUR, VC_REQUEST_ERROR_CODE_CONTROL << 8, (uchar)term, &val, 1) <= 0)
		return;
	if(val < nelem(str))
		werrstr("%s", str[val]);
}

int
infocheck(Cam *c, int term, Param *p)
{
	uchar val;

	if(usbcmd(c->dev, 0xA1, GET_INFO, p->cs << 8, term, &val, 1) <= 0){ errorcode(c->dev, term); return -1; }
	if((val & 1) == 0){
		werrstr("GET not supported");
		return -1;
	}
	return 0;
}

char *
pboolread(Cam *c, int term, Param *p)
{
	uchar val;
	Dev *d;

	d = c->dev;
	if(infocheck(c, term, p) < 0) return nil;
	if(usbcmd(d, 0xA1, GET_CUR, p->cs << 8, term, &val, 1) <= 0){ errorcode(d, term); return nil; }
	if(val)
		return strdup("true");
	return strdup("false");
}

int
pboolwrite(Cam *c, int term, Param *p, char **f, int nf)
{
	uchar v0, v1;

	if(nf != 1)
		return -1;
	v0 = cistrcmp(f[0], "false") == 0 || cistrcmp(f[0], "0") == 0 || cistrcmp(f[0], "no") == 0;
	v1 = cistrcmp(f[0], "true") == 0 || cistrcmp(f[0], "1") == 0 || cistrcmp(f[0], "yes") == 0;
	if(!(v0 ^ v1))
		return -1;
	if(usbcmd(c->dev, 0x21, SET_CUR, p->cs << 8, term, &v1, 1) <= 0){ errorcode(c->dev, term); return -1; }
	return 0;
}

char *
pintread(Cam *c, int term, Param *p)
{
	uchar cur[4], min[4], max[4], res[4];
	Dev *d;

	d = c->dev;
	if(infocheck(c, term, p) < 0) return nil;
	if(usbcmd(d, 0xA1, GET_CUR, p->cs << 8, term, cur, p->len) < p->len){ errorcode(d, term); return nil; }
	if(usbcmd(d, 0xA1, GET_MIN, p->cs << 8, term, min, p->len) < p->len){ errorcode(d, term); return nil; }
	if(usbcmd(d, 0xA1, GET_RES, p->cs << 8, term, res, p->len) < p->len){ errorcode(d, term); return nil; }
	if(usbcmd(d, 0xA1, GET_MAX, p->cs << 8, term, max, p->len) < p->len){ errorcode(d, term); return nil; }
	switch(p->len){
	case 1: return smprint("%d %d/%d/%d", (char)cur[0], (char)min[0], (char)res[0], (char)max[0]);
	case 2: return smprint("%d %d/%d/%d", (short)GET2(cur), (short)GET2(min), (short)GET2(res), (short)GET2(max));
	case 4: return smprint("%d %d/%d/%d", (int)GET4(cur), (int)GET4(min), (int)GET4(res), (int)GET4(max));
	}
	werrstr("pintread: unimplemented length %d", p->len);
	return nil;
}

int
pintwrite(Cam *c, int term, Param *p, char **f, int nf)
{
	int v;
	char *sp;
	uchar buf[4];
	
	if(nf != 1) return -1;
	v = strtol(f[0], &sp, 0);
	if(*f[0] == 0 || *sp != 0) return -1;
	buf[0] = v;
	buf[1] = v >> 8;
	buf[2] = v >> 16;
	buf[3] = v >> 24;
	if(usbcmd(c->dev, 0x21, SET_CUR, p->cs << 8, term, buf, p->len) < p->len){ errorcode(c->dev, term); return -1; }
	return 0;
}

char *
puintread(Cam *c, int term, Param *p)
{
	uchar cur[4], min[4], max[4], res[4];
	Dev *d;

	d = c->dev;
	if(infocheck(c, term, p) < 0) return nil;
	if(usbcmd(d, 0xA1, GET_CUR, p->cs << 8, term, cur, p->len) < p->len){ errorcode(d, term); return nil; }
	if(usbcmd(d, 0xA1, GET_MIN, p->cs << 8, term, min, p->len) < p->len){ errorcode(d, term); return nil; }
	if(usbcmd(d, 0xA1, GET_RES, p->cs << 8, term, res, p->len) < p->len){ errorcode(d, term); return nil; }
	if(usbcmd(d, 0xA1, GET_MAX, p->cs << 8, term, max, p->len) < p->len){ errorcode(d, term); return nil; }
	switch(p->len){
	case 1: return smprint("%ud %ud/%ud/%ud", (uchar)cur[0], (uchar)min[0], (uchar)res[0], (uchar)max[0]);
	case 2: return smprint("%ud %ud/%ud/%ud", (ushort)GET2(cur), (ushort)GET2(min), (ushort)GET2(res), (ushort)GET2(max));
	case 4: return smprint("%ud %ud/%ud/%ud", (uint)GET4(cur), (uint)GET4(min), (uint)GET4(res), (uint)GET4(max));
	}
	werrstr("pintread: unimplemented length %d", p->len);
	return nil;
}

int
puintwrite(Cam *c, int term, Param *p, char **f, int nf)
{
	uint v;
	char *sp;
	uchar buf[4];
	
	if(nf != 1) return -1;
	v = strtoul(f[0], &sp, 0);
	if(*f[0] == 0 || *sp != 0) return -1;
	buf[0] = v;
	buf[1] = v >> 8;
	buf[2] = v >> 16;
	buf[3] = v >> 24;
	if(usbcmd(c->dev, 0x21, SET_CUR, p->cs << 8, term, buf, p->len) < p->len){ errorcode(c->dev, term); return -1; }
	return 0;
}

char *
penumread(Cam *c, int term, Param *p)
{
	uchar cur[4];
	uint val;

	if(infocheck(c, term, p) < 0) return nil;
	if(usbcmd(c->dev, 0xA1, GET_CUR, p->cs << 8, term, cur, p->len) < p->len){ errorcode(c->dev, term); return nil; }
	switch(p->len){
	case 1: val = cur[0]; break;
	case 2: val = GET2(cur); break;
	case 4: val = GET4(cur); break;
	default:
		werrstr("pintread: unimplemented length %d", p->len);
		return nil;
	}
	if(val >= p->auxi || ((char**)p->auxp)[val] == nil)
		return smprint("%d", val);
	return smprint("%s", ((char**)p->auxp)[val]);
}

int
penumwrite(Cam *c, int term, Param *p, char **f, int nf)
{
	uint i;
	uchar buf[4];

	if(nf != 1) return -1;
	for(i = 0; i < p->auxi; i++)
		if(cistrcmp(((char**)p->auxp)[i], f[0]) == 0)
			break;
	if(i == p->auxi)
		return -1;
	buf[0] = i;
	buf[1] = i >> 8;
	buf[2] = i >> 16;
	buf[3] = i >> 24;
	if(usbcmd(c->dev, 0x21, SET_CUR, p->cs << 8, term, buf, p->len) < p->len){ errorcode(c->dev, term); return -1; }
	return 0;
}

char *
pformatread(Cam *c, int, Param *)
{
	Format *f;
	VSUncompressedFrame *g;
	char buf[5];
	
	if(c->pc.bFormatIndex >= c->nformat) goto nope;
	f = c->format[c->pc.bFormatIndex];
	if(f == nil) goto nope;
	if(c->pc.bFrameIndex >= f->nframe) goto nope;
	g = f->frame[c->pc.bFrameIndex];
	if(g == nil) goto nope;
	memcpy(buf, f->desc->guidFormat, 4);
	buf[4] = 0;
	return smprint("%dx%dx%d-%s", GET2(g->wWidth), GET2(g->wHeight), f->desc->bBitsPerPixel, buf);
nope:
	return smprint("#%d,%d", c->pc.bFormatIndex, c->pc.bFrameIndex);
}

void
frameinterval(Cam *c, VSUncompressedFrame *f, double t)
{
	double δ, minδ;
	int i, mini;
	uint min, max, step, val;

	if(f->bFrameIntervalType == 0){
		min = GET4(f->dwFrameInterval[0]);
		max = GET4(f->dwFrameInterval[1]);
		step = GET4(f->dwFrameInterval[2]);
		if(t <= min)
			val = min;
		else if(t >= max)
			val = max;
		else{
			val = floor((t - min) / step) * step + min;
			if(t >= val + step / 2.0)
				t += step;
		}
	}else{
		mini = -1;
		for(i = 0; i < f->bFrameIntervalType; i++){
			δ = fabs(((u32int)GET4(f->dwFrameInterval[i])) - t);
			if(mini < 0 || δ < minδ){
				mini = i;
				minδ = δ;
			}
		}
		assert(mini >= 0);
		val = GET4(f->dwFrameInterval[mini]);
	}
	PUT4(c->pc.dwFrameInterval, val);
}

int
findres(Cam *c, int w, int h, int fr)
{
	Format *f;
	VSUncompressedFrame *g;
	int i;

	if(fr >= c->nformat || (f = c->format[fr]) == nil) return -1;
	for(i = 0; i < f->nframe; i++){
		g = f->frame[i];
		if(g == nil) continue;
		if(GET2(g->wWidth) == w && GET2(g->wHeight) == h)
			return i;
	}
	return -1;
}

int
pformatwrite(Cam *c, int, Param *, char **args, int nargs)
{
	int w, h, bpp;
	char *p;
	int i;
	int j;
	char *q;
	Format *f;

	if(nargs != 1) return -1;
	p = args[0];
	if(*p == 0) return -1;
	w = strtol(p, &p, 0);
	if(*p != 'x') return -1;
	h = strtol(p + 1, &q, 0);
	if(q == p + 1) return -1;
	p = q;
	if(*p == 0){
		j = c->pc.bFormatIndex;
		i = findres(c, w, h, j);
		if(i < 0)
			for(j = 0; j < c->nformat; j++){
				i = findres(c, w, h, j);
				if(i >= 0) break;
			}
	}else{
		if(*p != 'x' || *++p == '-') return -1;
		bpp = strtol(p, &p, 0);
		if(*p != '-') return -1;
		if(strlen(p) != 4) return -1;
		i = -1;
		for(j = 0; j < c->nformat; j++){
			if((f = c->format[j]) == nil) continue;
			if(f->desc->bBitsPerPixel != bpp) continue;
			if(memcmp(f->desc->guidFormat, p, 4) != 0) continue;
			i = findres(c, w, h, j);
			if(i >= 0) break;
		}
	}
	if(i < 0)
		return -1;
	if(c->active != 0){
		werrstr("camera active");
		return -1;
	}
	c->pc.bFormatIndex = j;
	c->pc.bFrameIndex = i;
	frameinterval(c, c->format[j]->frame[i], GET4(c->pc.dwFrameInterval));
	return 0;
}

char *
pfpsread(Cam *c, int, Param *)
{
	if(GET4(c->pc.dwFrameInterval) == 0)
		return smprint("?");
	return smprint("%.2f", 10e6 / GET4(c->pc.dwFrameInterval));
}

int
pfpswrite(Cam *c, int, Param *, char **args, int nargs)
{
	double d, t;
	char *sp;
	VSUncompressedFrame *f;
	
	if(nargs != 1) return -1;
	d = strtod(args[0], &sp);
	if(*args[0] == 0 || *sp != 0) return -1;
	if(getframedesc(c, c->pc.bFormatIndex, c->pc.bFrameIndex, nil, &f) < 0){
		werrstr("invalid format active");
		return -1;
	}
	if(isNaN(d) || isInf(d, 1) || d <= 0) return -1;
	if(c->active != 0){
		werrstr("camera active");
		return -1;
	}
	t = 10e6 / d;
	frameinterval(c, f, t);
	return 0;
}

//static char *autoexposure[] = {"manual", "auto", "shutter", "aperture"};
static char *powerlinefrequency[] = {"disabled", "50", "60", "auto"};

static Param params[] = {
	{"format", PARAMSPEC, -1, -1, -1, pformatread, pformatwrite},
	{"fps", PARAMSPEC, -1, -1, -1, pfpsread, pfpswrite},
	{"progressive", PARAMCT, CT_SCANNING_MODE_CONTROL, 1, 0, pboolread, pboolwrite},
//	{"auto-exposure-mode", PARAMCT, CT_AE_MODE_CONTROL, 1, 1, pbitread, pbitwrite, autoexposure, nelem(autoexposure)},
	{"auto-exposure-priority", PARAMCT, CT_AE_PRIORITY_CONTROL, 1, 2, pboolread, pboolwrite},
	{"exposure-time", PARAMCT, CT_EXPOSURE_TIME_ABSOLUTE_CONTROL, 4, 3, puintread, puintwrite},
	{"focus", PARAMCT, CT_FOCUS_ABSOLUTE_CONTROL, 2, 5, puintread, puintwrite},
	{"focus-simple", PARAMCT, CT_FOCUS_SIMPLE_CONTROL, 1, 19, puintread, puintwrite},
	{"focus-auto", PARAMCT, CT_FOCUS_AUTO_CONTROL, 1, 17, pboolread, pboolwrite},
	{"iris", PARAMCT, CT_IRIS_ABSOLUTE_CONTROL, 2, 7, puintread, puintwrite},
	{"zoom", PARAMCT, CT_ZOOM_ABSOLUTE_CONTROL, 2, 9, puintread, puintwrite},
	{"backlight-compensation", PARAMPU, PU_BACKLIGHT_COMPENSATION_CONTROL, 2, 8, puintread, puintwrite},
	{"brightness", PARAMPU, PU_BRIGHTNESS_CONTROL, 2, 0, pintread, pintwrite},
	{"contrast", PARAMPU, PU_CONTRAST_CONTROL, 2, 1, puintread, puintwrite},
	{"contrast-auto", PARAMPU, PU_CONTRAST_AUTO_CONTROL, 1, 18, pboolread, pboolwrite},
	{"gain", PARAMPU, PU_GAIN_CONTROL, 2, 9, puintread, puintwrite},
	{"powerline-frequency", PARAMPU, PU_POWER_LINE_FREQUENCY_CONTROL, 1, 10, penumread, penumwrite, powerlinefrequency, nelem(powerlinefrequency)},
	{"hue", PARAMPU, PU_HUE_CONTROL, 2, 2, pintread, pintwrite},
	{"hue-auto", PARAMPU, PU_HUE_AUTO_CONTROL, 1, 11, pboolread, pboolwrite},
	{"saturation", PARAMPU, PU_SATURATION_CONTROL, 2, 3, puintread, puintwrite},
	{"sharpness", PARAMPU, PU_SHARPNESS_CONTROL, 2, 4, puintread, puintwrite},
	{"gamma", PARAMPU, PU_GAMMA_CONTROL, 2, 5, puintread, puintwrite},
	{"white-balance-temperature", PARAMPU, PU_WHITE_BALANCE_TEMPERATURE_CONTROL, 2, 6, puintread, puintwrite},
	{"white-balance-temperature-auto", PARAMPU, PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL, 1, 12, pboolread, pboolwrite},
};

int
unittype(int i, uchar **ctlp)
{
	if(unit[i] == nil)
		return -1;
	switch(unit[i]->bDescriptorSubtype){
	case VC_INPUT_TERMINAL:
		if(GET2(((VCInputTerminal*)unit[i])->wTerminalType) == ITT_CAMERA){
			if(ctlp != nil) *ctlp = ((VCCameraTerminal*)unit[i])->bmControls;
			return PARAMCT;
		}
		break;
	case VC_PROCESSING_UNIT:
		if(ctlp != nil) *ctlp = ((VCProcessingUnit*)unit[i])->bmControls;
		return PARAMPU;
	}
	return -1;
}

char *
ctlread(Cam *c)
{
	Fmt f;
	int i;
	int ut;
	Param *p;
	uchar *bmControls;
	int ifid;
	char *str;
	
	fmtstrinit(&f);
	for(p = params; p < params + nelem(params); p++){
		if(p->type != PARAMSPEC) continue;
		str = p->read(c, c->iface->id, p);
		if(str == nil)
			continue;
		fmtprint(&f, "0 %s %s\n", p->name, str);
		free(str);
	}
	for(i = 0; i < nunit; i++){
		ut = unittype(i, &bmControls);
		if(ut < 0) continue;
		ifid = unitif[i]->id;
		for(p = params; p < params + nelem(params); p++){
			if(p->type != ut) continue;
			if(bmControls != nil && p->flag >= 0 && (bmControls[p->flag >> 3] & 1<<(p->flag & 7)) == 0)
				continue;
			str = p->read(c, i << 8 | ifid, p);
			if(str == nil)
				continue;
			fmtprint(&f, "%d %s %s\n", i, p->name, str);
			free(str);
		}
	}
	return fmtstrflush(&f);
}

static Param *
findparam(char *s)
{
	Param *p;
	
	for(p = params; p < params + nelem(params); p++)
		if(strcmp(s, p->name) == 0)
			return p;
	werrstr("no such parameter");
	return nil;
}

static int
unitbytype(int type)
{
	int i;

	for(i = 0; i < nunit; i++)
		if(unittype(i, nil) == type)
			return i;
	werrstr("no matching unit");
	return -1;
}

int
ctlwrite(Cam *c, char *msg)
{
	char *f[10], *sp;
	uchar *bmControls;
	Param *p;
	int aut;
	int nf;
	int uid, ifid;
	
	nf = tokenize(msg, f, nelem(f));
	if(nf == nelem(f))
		return -1;
	uid = strtoul(f[0], &sp, 0);
	aut = *f[0] == 0 || *sp != 0;
	if(aut){
		p = findparam(f[0]);
		if(p == nil)
			return -1;
		if(p->type == PARAMSPEC)
			uid = 0;
		else
			uid = unitbytype(p->type);
		if(uid < 0)
			return -1;
	}else{
		p = findparam(f[1]);
		if(p == nil)
			return -1;
		if(p->type != PARAMSPEC && ((uint)uid >= nunit || unit[uid] == nil)){
			werrstr("no such unit");
			return -1;
		}
	}
	if(p->type != PARAMSPEC){
		if(unittype(uid, &bmControls) != p->type){
			werrstr("unit does not have this parameter");
			return -1;
		}
		if(bmControls != nil && p->flag >= 0 && (bmControls[p->flag >> 3] & 1<<(p->flag & 7)) == 0){
			werrstr("parameter not available");
			return -1;
		}
		ifid = unitif[uid]->id;
	}else
		ifid = c->iface->id;
	if(p->write == nil){
		werrstr("read-only parameter");
		return -1;
	}
	return p->write(c, uid << 8 | ifid, p, f + (2 - aut), nf - (2 - aut));
}
