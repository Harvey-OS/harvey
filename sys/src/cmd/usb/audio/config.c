#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbproto.h"
#include "dat.h"
#include "fns.h"
#include "audioclass.h"

static Stream *
getstream(Audiofunc *af, int i)
{
	int j;
	Dalt *alt;
	Dconf *c;
	Dinf *intf;
	Stream *s;
	Endpt *ep;
	uchar *p, *pe, *q;
	Streamalt *salt, *salttail;

	for(s = af->streams; s != nil; s = s->next)
		if(s->intf->x == i)
			return s;

	c = af->intf->conf;
	intf = &c->iface[i];

	s = emallocz(sizeof(Stream), 1);
	s->intf = intf;
	salttail = nil;
	for(alt = intf->alt; alt != nil; alt = alt->next) {
		salt = emallocz(sizeof(Streamalt), 1);
		salt->dalt = alt;
		salt->next = nil;
		if(salttail == nil)
			s->salt = salt;
		else
			salttail->next = salt;
		salttail = salt;
		p = alt->desc.data;
		pe = p + alt->desc.bytes;
		while(p < pe) {
			if(p[1] == CS_INTERFACE) {
				switch(p[2]) {
				case AS_GENERAL:
					s->id = p[3];
					salt->delay = p[4];
					salt->format = GET2(&p[5]);
					break;
				case FORMAT_TYPE:
					switch(p[3]) {
					case FORMAT_TYPE_I:
						salt->nchan = p[4];
						salt->subframe = p[5];
						salt->res = p[6];
						salt->nf = p[7];
						if(salt->nf == 0) {
							salt->minf = p[8]|(p[9]<<8)|(p[10]<<16);
							salt->maxf = p[11]|(p[12]<<8)|(p[13]<<16);
						}
						else {
							salt->f = emalloc(salt->nf*sizeof(int));
							for(j = 0; j < salt->nf; j++) {
								q = p+8+3*j;
								salt->f[j] = q[0]|(q[1]<<8)|(q[2]<<16);
							}
						}
						break;
					default:
						// werrstr("format type %d not handled", p[3]);
						salt->unsup = 1;
						break;
					}
					break;
				case FORMAT_SPECIFIC:
					break;
				}
			}
			p += p[0];
		}
		if(alt->npt > 0) {
			ep = &alt->ep[0];
			p = ep->desc.data;
			pe = p + ep->desc.bytes;
			while(p < pe) {
				if(p[1] = CS_ENDPOINT && p[2] == EP_GENERAL) {
					salt->attr = p[3];
					salt->lockunits = p[4];
					salt->lockdelay = GET2(&p[5]);
				}
				p += p[0];
			}
		}
	}
	s->next = af->streams;
	af->streams = s;
	return s;
}

static Stream *
findstream(Funcalt *falt, int id)
{
	int i;
	Stream *s;

	for(i = 0; i < falt->nstream; i++) {
		s = falt->stream[i];
		if(s->id == id)
			return s;
	}
	return nil;
}

Audiofunc *
getaudiofunc(Dinf *intf)
{
	Dalt *alt;
	int len, i, j, sz;
	Audiofunc *af;
	uchar *p, *pe, *q;
	Unit *u, *utail, *src;
	Funcalt *falt, *falttail;

	af = emallocz(sizeof(Audiofunc), 1);
	af->intf = intf;

	falttail = nil;
	for(alt = intf->alt; alt != nil; alt = alt->next) {
		/* scan for audiocontrol ``header'' descriptor */
		p = alt->desc.data;
		pe = p + alt->desc.bytes;
		while(p < pe) {
			if(p[1] == CS_INTERFACE && p[2] == HEADER)
				break;
			p += p[0];
		}
		if(p >= pe)
			continue;
		len = GET2(&p[5]);
		if(len > pe-p)
			len = pe-p;
		if(p[0] > len || 8+p[7] > p[0]) {
			werrstr("truncated descriptor");
			/* BUG: memory leak */
			return nil;
		}

		falt = emallocz(sizeof(Funcalt), 1);
		falt->dalt = alt;
		falt->next = nil;
		if(falttail == nil)
			af->falt = falt;
		else
			falttail->next = falt;
		falttail = falt;
		falt->nstream = p[7];
		falt->stream = emallocz(falt->nstream*sizeof(Stream*), 1);
		for(i = 0; i < falt->nstream; i++) {
			if((falt->stream[i] = getstream(af, p[8+i])) == nil) {
				werrstr("getstream: %r");
				return nil;
			}
		}
		pe = p+len;
		p += p[0];
		utail = nil;
		for(; p < pe; p += p[0]) {
			if(p[1] != CS_INTERFACE)
				continue;
			/* value doesn't appear in standard...
			if(p[2] == ASSOC_INTERFACE) {
				if(utail != nil)
					utail->associf = p[3];
				continue;
			}
			*/
			u = emallocz(sizeof(Unit), 1);
			u->type = p[2];
			u->id = p[3];
			if(utail == nil)
				falt->unit = u;
			else
				utail->next = u;
			utail = u;
			switch(u->type) {
			case INPUT_TERMINAL:
				u->termtype = GET2(&p[4]);
				u->assoc = p[6];
				u->nchan = p[7];
				u->chanmask = GET2(&p[8]);
				break;
			case OUTPUT_TERMINAL:
				u->termtype = GET2(&p[4]);
				u->assoc = p[6];
				u->nsource = 1;
				u->sourceid = emalloc(sizeof(int));
				u->sourceid[0] = p[7];
				break;
			case MIXER_UNIT:
				u->nsource = p[4];
				u->sourceid = emalloc(u->nsource*sizeof(int));
				for(i = 0; i < u->nsource; i++)
					u->sourceid[i] = p[5+i];
				q = p+5+u->nsource;
				u->nchan = q[0];
				u->chanmask = GET2(&q[1]);
				/* TODO: decode bmControls bitmap */
				break;
			case SELECTOR_UNIT:
				u->nsource = p[4];
				u->sourceid = emalloc(u->nsource*sizeof(int));
				for(i = 0; i < u->nsource; i++)
					u->sourceid[i] = p[5+i];
				break;
			case FEATURE_UNIT:
				u->nsource = 1;
				u->sourceid = emalloc(sizeof(int));
				u->sourceid[0] = p[4];
				/* need nchan before we can decode bitmaps, defer to later pass */
				u->fdesc = p;
				break;
			case PROCESSING_UNIT:
				u->proctype = GET2(&p[4]);
				u->nsource = p[6];
				u->sourceid = emalloc(u->nsource*sizeof(int));
				for(i = 0; i < u->nsource; i++)
					u->sourceid[i] = p[7+i];
				q = p+7+u->nsource;
				u->nchan = q[0];
				u->chanmask = GET2(&q[1]);
				/* TODO: decode bmControls bitmap and processing-specific section */
				break;
			case EXTENSION_UNIT:
				u->exttype = GET2(&p[4]);
				u->nsource = p[6];
				u->sourceid = emalloc(u->nsource*sizeof(int));
				for(i = 0; i < u->nsource; i++)
					u->sourceid[i] = p[7+i];
				q = p+7+u->nsource;
				u->nchan = q[0];
				u->chanmask = GET2(&q[1]);
				/* TODO: decode bmControls bitmap */
				break;
			}
		}
		/* resolve source references */
		for(u = falt->unit; u != nil; u = u->next) {
			if(u->type == INPUT_TERMINAL || u->type == OUTPUT_TERMINAL) {
				if(u->termtype == USB_STREAMING) {
					if((u->stream = findstream(falt, u->id)) == nil) {
						werrstr("stream for terminal id %d not found", u->id);
						return nil;
					}
				}
			}
			if(u->nsource == 0)
				continue;
			u->source = emallocz(u->nsource*sizeof(Unit*), 1);
			for(i = 0; i < u->nsource; i++) {
				for(src = falt->unit; src != nil; src = src->next)
					if(src->id == u->sourceid[i])
						break;
				if(src == nil) {
					werrstr("source id %d missing", u->sourceid[i]);
					return nil;
				}
				u->source[i] = src;
			}
		}
		/* fill in nchan and chanmask for units which derive it from upstream */
		for(u = falt->unit; u != nil; u = u->next) {
			src = u;
			for(;;) {
				if(src == nil)
					break;
				switch(src->type) {
				case OUTPUT_TERMINAL:
				case FEATURE_UNIT:
				case SELECTOR_UNIT:
					if(src->nsource == 0) {
						werrstr("unit id %d (type %d) needs a source", src->id, src->type);
						return nil;
					}
					src = src->source[0];
					break;
				default:
					if(src != u) {
						u->nchan = src->nchan;
						u->chanmask = src->chanmask;
					}
					goto break2;
				}
			}
			break2:;
		}
		/* finish decoding feature units */
		for(u = falt->unit; u != nil; u = u->next) {
			if(u->type == FEATURE_UNIT) {
				q = u->fdesc+5;
				u->hascontrol = emallocz((u->nchan+1)*sizeof(int), 1);
				sz = *q++;
				for(i = 0; i <= u->nchan; i++)
					for(j = 0; j < sz; j++)
						u->hascontrol[i] |= *q++ << (j*8);
			}
		}
	}
	return af;
}
