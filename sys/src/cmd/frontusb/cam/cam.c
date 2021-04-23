#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <bio.h>
#include "usb.h"
#include "uvc.h"
#include "dat.h"
#include "fns.h"

char user[] = "cam";

void printVCHeader(void *vp);
void printVCInputTerminal(void *vp);
void printVCOutputTerminal(void *vp);
void printVCCameraTerminal(void *vp);
void printVCSelectorUnit(void *vp);
void printVCProcessingUnit(void *vp);
void printVCEncodingUnit(void *vp);
void printVCExtensionUnit(void *vp);
void printVSInputHeader(void *vp);
void printVSOutputHeader(void *vp);
void printVSStillFrame(void *vp);
void printVSUncompressedFormat(void *vp);
void printVSUncompressedFrame(void *vp);
void printVSColorFormat(void *vp);
void printProbeControl(void *vp);

Cam *cams;
int nunit;
VCUnit **unit;
Iface **unitif;

int debug;

int
getframedesc(Cam *c, int i, int j, Format **fp, VSUncompressedFrame **gp)
{
	Format *f;

	if(i >= c->nformat) return -1;
	f = c->format[i];
	if(f == nil) return -1;
	if(j >= f->nframe) return -1;
	if(f->frame[j] == nil) return -1;
	if(fp != nil) *fp = f;
	if(gp != nil) *gp = f->frame[j];
	return 0;
}

static void
parsevcdesc(Dev *dev, Desc *d)
{
	VCDescriptor *vdp;
	static Cam *cam;
	static Format *format;
	int i;
	Format *f;
	
	if(d == nil) return;
	vdp = (VCDescriptor *) &d->data;
	if(vdp->bDescriptorType != 0x24) return;
	if(Class(d->iface->csp) != CC_VIDEO) return;
	switch(Subclass(d->iface->csp)){
	case SC_VIDEOSTREAMING:
		switch(vdp->bDescriptorSubtype){
		case VS_INPUT_HEADER:
			format = nil;
			cam = emallocz(sizeof(Cam), 1);
			cam->dev = dev;
			cam->iface = d->iface;
			cam->hdr = (VSInputHeader *) vdp;
			cam->next = cams;
			cams = cam;
			break;
		case VS_FORMAT_UNCOMPRESSED:
			if(cam == nil) return;
			f = emallocz(sizeof(Format), 1);
			f->desc = (void*)vdp;
			i = f->desc->bFormatIndex;
			if(i >= cam->nformat){
				cam->format = realloc(cam->format, (i + 1) * sizeof(void *));
				memset(cam->format + cam->nformat, 0, (i - cam->nformat) * sizeof(void *));
				cam->nformat = i + 1;
			}
			cam->format[i] = f;
			if(format == nil) cam->pc.bFormatIndex = i;
			format = f;
			break;
		case VS_FRAME_UNCOMPRESSED:
			if(cam == nil || cam->nformat == 0) return;
			f = format;
			i = ((VSUncompressedFrame*)vdp)->bFrameIndex;
			if(i >= f->nframe){
				f->frame = realloc(f->frame, (i + 1) * sizeof(void *));
				memset(f->frame + f->nframe, 0, (i - f->nframe) * sizeof(void *));
				f->nframe = i + 1;
			}
			f->frame[i] = (void*)vdp;
			break;
		}
		break;
	case SC_VIDEOCONTROL:
		switch(vdp->bDescriptorSubtype){
		case VC_INPUT_TERMINAL:
		case VC_OUTPUT_TERMINAL:
		case VC_SELECTOR_UNIT:
		case VC_PROCESSING_UNIT:
		case VC_ENCODING_UNIT:
		case VC_EXTENSION_UNIT:
			i = ((VCUnit*)vdp)->bUnitID;
			if(i >= nunit){
				unit = realloc(unit, (i + 1) * sizeof(void *));
				unitif = realloc(unitif, (i + 1) * sizeof(Iface *));
				memset(unit + nunit, 0, (i - nunit) * sizeof(void *)); 
				memset(unitif + nunit, 0, (i - nunit) * sizeof(void *));
				nunit = i + 1;
			}
			unit[i] = (void*)vdp;
			unitif[i] = d->iface;
			break;
		}
		break;
	}
}

static void
createfiles(File *root, char *devname, Cam *c)
{
	char buf[512];
	File *d;
	
	snprint(buf, sizeof(buf), "cam%s.%d", devname, c->iface->id);
	d = createfile(root, buf, user, DMDIR|0555, c);
	c->ctlfile = createfile(d, "ctl", user, 0666, c);
	c->formatsfile = createfile(d, "formats", user, 0444, c);
	c->videofile = createfile(d, "video", user, 0444, c);
	c->framefile = createfile(d, "frame", user, 0444, c);
	c->descfile = createfile(d, "desc", user, 0444, c);
}

static char *
formatread(Cam *c)
{
	int i, j, k;
	Fmt fmt;
	Format *f;
	VSUncompressedFrame *g;
	char buf[5];
	
	fmtstrinit(&fmt);
	for(i = 0; i < c->nformat; i++){
		f = c->format[i];
		if(f == nil) continue;
		memcpy(buf, f->desc->guidFormat, 4);
		buf[4] = 0;
		for(j = 0; j < f->nframe; j++){
			if((g = f->frame[j]) == nil) continue;
			fmtprint(&fmt, "%dx%dx%d-%s ", GET2(g->wWidth), GET2(g->wHeight), f->desc->bBitsPerPixel, buf);
			if(g->bFrameIntervalType == 0)
				fmtprint(&fmt, "%.2f-%.2f\n", 10e6 / (u32int)GET4(g->dwFrameInterval[0]), 10e6 / (u32int)GET4(g->dwFrameInterval[1]));
			else
				for(k = 0; k < g->bFrameIntervalType; k++)
					fmtprint(&fmt, "%.2f%c", 10e6 / (u32int)GET4(g->dwFrameInterval[k]), k == g->bFrameIntervalType - 1 ? '\n' : ',');
		}
	}
	return fmtstrflush(&fmt);
}

static char *
descread(Cam *c)
{
	Fmt fmt;
	int i;
	Usbdev *ud;
	Desc *d;
	VCDescriptor *vdp;

	ud = c->dev->usb;
	fmtstrinit(&fmt);
	for(i = 0; i < nelem(ud->ddesc); i++){
		d = ud->ddesc[i];
		if(d == nil) continue;
		vdp = (VCDescriptor *) &d->data;
		if(vdp->bDescriptorType != 0x24) continue;
		if(Class(d->iface->csp) != CC_VIDEO) continue;
		printDescriptor(&fmt, d->iface, vdp);
	}
	return fmtstrflush(&fmt);
}

typedef struct ReadState ReadState;
struct ReadState {
	int len;
	char *buf;
};

static void
strread(Req *req, char *str, int len)
{
	ReadState *rs;
	
	if(req->fid->aux != nil){
		free(((ReadState*)req->fid->aux)->buf);
		free(req->fid->aux);
		req->fid->aux = nil;
	}
	if(str == nil)
		return;
	rs = emallocz(sizeof(ReadState), 1);
	rs->len = len < 0 ? strlen(str) : len;
	rs->buf = str;
	req->fid->aux = rs;
}

static void
fsread(Req *req)
{
	File *f;
	Cam *c;
	ReadState *rs;

	if(req->fid == nil || req->fid->file == nil || req->fid->file->aux == nil){
		respond(req, "the front fell off");
		return;
	}
	f = req->fid->file;
	c = f->aux;
	if(req->fid->aux == nil || req->ifcall.offset == 0)
		if(f == c->formatsfile)
			strread(req, formatread(c), -1);
		else if(f == c->ctlfile)
			strread(req, ctlread(c), -1);
		else if(f == c->descfile)
			strread(req, descread(c), -1);
		else if(f == c->videofile || f == c->framefile){
			videoread(req, c, 1);
			return;
		}
	if((rs = req->fid->aux) == nil){
		respond(req, "the front fell off");
		return;
	}
	readbuf(req, rs->buf, rs->len);
	respond(req, nil);
}

static void
fswrite(Req *req)
{
	File *f;
	Cam *c;
	char *s;
	int n;

	if(req->fid == nil || req->fid->file == nil || req->fid->file->aux == nil){
err:		respond(req, "the front fell off");
		return;
	}
	f = req->fid->file;
	c = f->aux;
	if(f != c->ctlfile)
		goto err;
	for(n = 0; n < req->ifcall.count; n++)
		if(req->ifcall.data[n] == '\n')
			break;
	s = emallocz(n+1, 0);
	memcpy(s, req->ifcall.data, n);
	s[n] = 0;
	werrstr("invalid argument");
	if(ctlwrite(c, s) < 0)
		responderror(req);
	else{
		req->ofcall.count = req->ifcall.count;
		respond(req, nil);
	}
	free(s);
}

static void
fsdestroyfid(Fid *fid)
{
	ReadState *rs;

	if(fid->omode == -1)
		return;
	rs = fid->aux;
	if(rs != nil){
		free(rs->buf);
		free(rs);
	}
	if(fid->file != nil && fid->file->aux != nil && (fid->file == ((Cam*)fid->file->aux)->videofile || fid->file == ((Cam*)fid->file->aux)->framefile))
		videoclose(fid->file->aux);
}

static void
fsopen(Req *req)
{
	File *f;
	Cam *c;

	if((req->ofcall.qid.type & QTDIR) != 0){
		respond(req, nil);
		return;
	}
	if(req->fid == nil || req->fid->file == nil || req->fid->file->aux == nil){
		respond(req, "the front fell off");
		return;
	}
	f = req->fid->file;
	c = f->aux;
	if(f == c->videofile || f == c->framefile)
		if(videoopen(c, f == c->framefile) < 0){
			responderror(req);
			return;
		}
	respond(req, nil);
}

static void
fsflush(Req *req)
{
	if(req->oldreq->fid->file != nil && req->oldreq->fid->file->aux != nil && (((Cam*)req->oldreq->fid->file->aux)->videofile == req->oldreq->fid->file || ((Cam*)req->oldreq->fid->file->aux)->framefile == req->oldreq->fid->file))
		videoflush(req->oldreq, req->oldreq->fid->file->aux);
	respond(req, nil);
}

static void
usage(void)
{
	fprint(2, "usage: %s [-d] devid\n", argv0);
	threadexits("usage");
}

static Srv fs = {
	.open = fsopen,
	.read = fsread,
	.write = fswrite,
	.flush = fsflush,
	.destroyfid = fsdestroyfid,
};

void
threadmain(int argc, char* argv[])
{
	int i;
	Dev *d;
	Usbdev *ud;
	char buf[512];
	Cam *c;

	ARGBEGIN{
	case 'd':
		debug++;
		break;
	default:
		usage();
	}ARGEND;
	if(argc != 1)
		usage();
	d = getdev(*argv);
	if(d == nil)
		sysfatal("getdev: %r");
	ud = d->usb;
	for(i = 0; i < nelem(ud->ddesc); i++)
		parsevcdesc(d, ud->ddesc[i]);
	if(cams == nil)
		sysfatal("no input streams");
	for(c = cams; c != nil; c = c->next)
		if(c->nformat > 0){
			c->pc.bFrameIndex = c->format[c->pc.bFormatIndex]->desc->bDefaultFrameIndex;
			if(c->pc.bFrameIndex < c->format[c->pc.bFormatIndex]->nframe && c->format[c->pc.bFormatIndex]->frame[c->pc.bFrameIndex] != nil)
				PUT4(c->pc.dwFrameInterval, GET4(c->format[c->pc.bFormatIndex]->frame[c->pc.bFrameIndex]->dwDefaultFrameInterval));
		}
	fs.tree = alloctree(user, "usb", DMDIR|0555, nil);
	for(c = cams; c != nil; c = c->next)
		createfiles(fs.tree->root, d->hname, c);
	snprint(buf, sizeof buf, "%d.cam", d->id);
	threadpostsharesrv(&fs, nil, "usb", buf);
	threadexits(nil);
}
