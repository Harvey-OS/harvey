/*
 * omap35 display subsystem (dss) device interface to screen.c.
 * implements #v/vgactl
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "../port/error.h"

#define	Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "screen.h"
// #include "gamma.h"

enum {
	Qdir,
	Qdss,
};

extern OScreen oscreen;
extern Settings settings[];
extern Omap3fb *framebuf;

static QLock dsslck;
static Dirtab dsstab[] = {
	".",		{Qdir, 0, QTDIR},	0,	0555|DMDIR,
	"vgactl",	{Qdss, 0},		0,	0666,
};

static Chan*
screenattach(char *spec)
{
	return devattach('v', spec);
}

static Walkqid*
screenwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, dsstab, nelem(dsstab), devgen);
}

static int
screenstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, dsstab, nelem(dsstab), devgen);
}

static Chan*
screenopen(Chan *c, int omode)
{
	if ((ulong)c->qid.path == Qdss) {
		qlock(&dsslck);
		oscreen.open = 1;
		c->mode = openmode(omode);
		c->flag |= COPEN;
		c->offset = 0;
	}
	return c;
}

static void
screenclose(Chan *c)
{
	if ((c->qid.type & QTDIR) == 0 && c->flag & COPEN)
		if (c->qid.path == Qdss) {
			oscreen.open = 0;
			qunlock(&dsslck);
		}
}

static ulong
getchans(char *p)
{
	if (strncmp("x24" , p, 3) == 0)
		return RGB24;		/* can't work yet, pixels are shorts */
	else if (strncmp("x16", p, 3) == 0)
		return RGB16;
	else
		return RGB16;
}

static long
settingswrite(OScreen *scr, char *p)
{
	if (strncmp("800x600", p, 7) == 0) {
		p += 7;
		scr->settings = &settings[Res800x600];
	} else if (strncmp("1024x768", p, 8) == 0) {
		p += 8;
		scr->settings = &settings[Res1024x768];
	} else if (strncmp("1280x1024", p, 9) == 0) {
		p += 9;
		scr->settings = &settings[Res1280x1024];
	} else
		return -1;
	scr->settings->chan = getchans(p);
	return 1;
}

static long
screenread(Chan *c, void *a, long n, vlong off)
{
	int len, depth;
	char *p;
	Settings *set;

	switch ((ulong)c->qid.path) {
	case Qdir:
		return devdirread(c, a, n, dsstab, nelem(dsstab), devgen);
	case Qdss:
		set = oscreen.settings;
		p = malloc(READSTR);
		if(waserror()){
			free(p);
			nexterror();
		}
		if (set->chan == RGB16)
			depth = 16;
		else if (set->chan == RGB24)
			depth = 24;
		else
			depth = 0;
		len = snprint(p, READSTR, "size %dx%dx%d @ %d Hz\n"
			"addr %#p size %ud\n", set->wid, set->ht, depth,
			set->freq, framebuf, sizeof *framebuf);
		USED(len);
		n = readstr(off, a, n, p);
		poperror();
		free(p);
		return n;
	default:
		error(Egreg);
	}
	return 0;
}

static long
screenwrite(Chan *c, void *a, long n, vlong off)
{
	switch ((ulong)c->qid.path) {
	case Qdss:
		if(off)
			error(Ebadarg);
		n = settingswrite(&oscreen, a);
		if (n < 0)
			error(Ebadctl);
		screeninit();
		return n;
	default:
		error(Egreg);
	}
	return 0;
}

Dev dssdevtab = {
	L'v',
	"dss",

	devreset,
	devinit,
	devshutdown,		// TODO add a shutdown to stop dma to monitor
	screenattach,
	screenwalk,
	screenstat,
	screenopen,
	devcreate,
	screenclose,
	screenread,
	devbread,
	screenwrite,
	devbwrite,
	devremove,
	devwstat,
};
