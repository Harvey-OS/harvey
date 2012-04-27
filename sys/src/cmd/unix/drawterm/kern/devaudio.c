#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"
#include	"devaudio.h"

enum
{
	Qdir		= 0,
	Qaudio,
	Qvolume,

	Aclosed		= 0,
	Aread,
	Awrite,

	Speed		= 44100,
	Ncmd		= 50,		/* max volume command words */
};

Dirtab
audiodir[] =
{
	".",	{Qdir, 0, QTDIR},		0,	DMDIR|0555,
	"audio",	{Qaudio},		0,	0666,
	"volume",	{Qvolume},		0,	0666,
};

static	struct
{
	QLock	lk;
	Rendez	vous;
	int	amode;		/* Aclosed/Aread/Awrite for /audio */
} audio;

#define aqlock(a) qlock(&(a)->lk)
#define aqunlock(a) qunlock(&(a)->lk)

static	struct
{
	char*	name;
	int	flag;
	int	ilval;		/* initial values */
	int	irval;
} volumes[] =
{
	"audio",	Fout, 		50,	50,
	"synth",	Fin|Fout,	0,	0,
	"cd",		Fin|Fout,	0,	0,
	"line",	Fin|Fout,	0,	0,
	"mic",	Fin|Fout|Fmono,	0,	0,
	"speaker",	Fout|Fmono,	0,	0,

	"treb",		Fout, 		50,	50,
	"bass",		Fout, 		50,	50,

	"speed",	Fin|Fout|Fmono,	Speed,	Speed,
	0
};

static	char	Emode[]		= "illegal open mode";
static	char	Evolume[]	= "illegal volume specifier";

static	void
resetlevel(void)
{
	int i;

	for(i=0; volumes[i].name; i++)
		audiodevsetvol(i, volumes[i].ilval, volumes[i].irval);
}

static void
audioinit(void)
{
}

static Chan*
audioattach(char *param)
{
	return devattach('A', param);
}

static Walkqid*
audiowalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, audiodir, nelem(audiodir), devgen);
}

static int
audiostat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, audiodir, nelem(audiodir), devgen);
}

static Chan*
audioopen(Chan *c, int omode)
{
	int amode;

	switch((ulong)c->qid.path) {
	default:
		error(Eperm);
		break;

	case Qvolume:
	case Qdir:
		break;

	case Qaudio:
		amode = Awrite;
		if((omode&7) == OREAD)
			amode = Aread;
		aqlock(&audio);
		if(waserror()){
			aqunlock(&audio);
			nexterror();
		}
		if(audio.amode != Aclosed)
			error(Einuse);
		audiodevopen();
		audio.amode = amode;
		poperror();
		aqunlock(&audio);
		break;
	}
	c = devopen(c, omode, audiodir, nelem(audiodir), devgen);
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;

	return c;
}

static void
audioclose(Chan *c)
{
	switch((ulong)c->qid.path) {
	default:
		error(Eperm);
		break;

	case Qdir:
	case Qvolume:
		break;

	case Qaudio:
		if(c->flag & COPEN) {
			aqlock(&audio);
			audiodevclose();
			audio.amode = Aclosed;
			aqunlock(&audio);
		}
		break;
	}
}

static long
audioread(Chan *c, void *v, long n, vlong off)
{
	int liv, riv, lov, rov;
	long m;
	char buf[300];
	int j;
	ulong offset = off;
	char *a;

	a = v;
	switch((ulong)c->qid.path) {
	default:
		error(Eperm);
		break;

	case Qdir:
		return devdirread(c, a, n, audiodir, nelem(audiodir), devgen);

	case Qaudio:
		if(audio.amode != Aread)
			error(Emode);
		aqlock(&audio);
		if(waserror()){
			aqunlock(&audio);
			nexterror();
		}
		n = audiodevread(v, n);
		poperror();
		aqunlock(&audio);
		break;

	case Qvolume:
		j = 0;
		buf[0] = 0;
		for(m=0; volumes[m].name; m++){
			audiodevgetvol(m, &lov, &rov);
			liv = lov;
			riv = rov;
			j += snprint(buf+j, sizeof(buf)-j, "%s", volumes[m].name);
			if((volumes[m].flag & Fmono) || (liv==riv && lov==rov)){
				if((volumes[m].flag&(Fin|Fout))==(Fin|Fout) && liv==lov)
					j += snprint(buf+j, sizeof(buf)-j, " %d", liv);
				else{
					if(volumes[m].flag & Fin)
						j += snprint(buf+j, sizeof(buf)-j,
							" in %d", liv);
					if(volumes[m].flag & Fout)
						j += snprint(buf+j, sizeof(buf)-j,
							" out %d", lov);
				}
			}else{
				if((volumes[m].flag&(Fin|Fout))==(Fin|Fout) &&
				    liv==lov && riv==rov)
					j += snprint(buf+j, sizeof(buf)-j,
						" left %d right %d",
						liv, riv);
				else{
					if(volumes[m].flag & Fin)
						j += snprint(buf+j, sizeof(buf)-j,
							" in left %d right %d",
							liv, riv);
					if(volumes[m].flag & Fout)
						j += snprint(buf+j, sizeof(buf)-j,
							" out left %d right %d",
							lov, rov);
				}
			}
			j += snprint(buf+j, sizeof(buf)-j, "\n");
		}
		return readstr(offset, a, n, buf);
	}
	return n;
}

static long
audiowrite(Chan *c, void *vp, long n, vlong off)
{
	long m;
	int i, v, left, right, in, out;
	Cmdbuf *cb;
	char *a;

	USED(off);
	a = vp;
	switch((ulong)c->qid.path) {
	default:
		error(Eperm);
		break;

	case Qvolume:
		v = Vaudio;
		left = 1;
		right = 1;
		in = 1;
		out = 1;
		cb = parsecmd(vp, n);
		if(waserror()){
			free(cb);
			nexterror();
		}

		for(i = 0; i < cb->nf; i++){
			/*
			 * a number is volume
			 */
			if(cb->f[i][0] >= '0' && cb->f[i][0] <= '9') {
				m = strtoul(cb->f[i], 0, 10);
				if(!out)
					goto cont0;
				if(left && right)
					audiodevsetvol(v, m, m);
				else if(left)
					audiodevsetvol(v, m, -1);
				else if(right)
					audiodevsetvol(v, -1, m);
				goto cont0;
			}

			for(m=0; volumes[m].name; m++) {
				if(strcmp(cb->f[i], volumes[m].name) == 0) {
					v = m;
					in = 1;
					out = 1;
					left = 1;
					right = 1;
					goto cont0;
				}
			}

			if(strcmp(cb->f[i], "reset") == 0) {
				resetlevel();
				goto cont0;
			}
			if(strcmp(cb->f[i], "in") == 0) {
				in = 1;
				out = 0;
				goto cont0;
			}
			if(strcmp(cb->f[i], "out") == 0) {
				in = 0;
				out = 1;
				goto cont0;
			}
			if(strcmp(cb->f[i], "left") == 0) {
				left = 1;
				right = 0;
				goto cont0;
			}
			if(strcmp(cb->f[i], "right") == 0) {
				left = 0;
				right = 1;
				goto cont0;
			}
			error(Evolume);
			break;
		cont0:;
		}
		free(cb);
		poperror();
		break;

	case Qaudio:
		if(audio.amode != Awrite)
			error(Emode);
		aqlock(&audio);
		if(waserror()){
			aqunlock(&audio);
			nexterror();
		}
		n = audiodevwrite(vp, n);
		poperror();
		aqunlock(&audio);
		break;
	}
	return n;
}

void
audioswab(uchar *a, uint n)
{
	ulong *p, *ep, b;

	p = (ulong*)a;
	ep = p + (n>>2);
	while(p < ep) {
		b = *p;
		b = (b>>24) | (b<<24) |
			((b&0xff0000) >> 8) |
			((b&0x00ff00) << 8);
		*p++ = b;
	}
}

Dev audiodevtab = {
	'A',
	"audio",

	devreset,
	audioinit,
	devshutdown,
	audioattach,
	audiowalk,
	audiostat,
	audioopen,
	devcreate,
	audioclose,
	audioread,
	devbread,
	audiowrite,
	devbwrite,
	devremove,
	devwstat,
};
