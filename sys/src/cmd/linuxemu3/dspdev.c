#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

enum {
	FREQUENCY = 44100,
	CHANNELS = 2,
	DELAY = 100,
	FRAGSIZE = 4096,
};

typedef struct Chan Chan;
typedef struct DSP DSP;

struct Chan
{
	ulong	phase;
	int		last;
};

struct DSP
{
	Ufile;

	int		channels;		/* number of channels (2 for stereo) */
	int		freq;			/* frequency of sound stream */

	int		rfreq;		/* frequency of /dev/audio */

	uchar	*buf;			/* resampling */
	ulong	nbuf;
	Chan	chan[CHANNELS];	

	vlong	time;			/* time point of the last sample in device buffer */

	ulong	written;		/* number of bytes written to dsp */
	ulong	written2;		/* same as written, will be reset on every GETOPTR ioctl */
};

static int
closedsp(Ufile *file)
{
	DSP *dsp = (DSP*)file;

	trace("dsp: closedsp");
	free(dsp->buf);
	close(dsp->fd);

	return 0;
}

static int
polldsp(Ufile *, void *)
{
	return POLLOUT;
}

static int
readdsp(Ufile *, void *, int, vlong)
{
	return 0;		/* not implemented */
}

static int
resample(Chan *c, uchar *src, uchar *dst, int sstep, int dstep, ulong delta, ulong count)
{
	int last, val, out;
	ulong phase, pos;
	uchar *dp, *sp;

	dp = dst;
	last = val = c->last;
	phase = c->phase;
	pos = phase >> 16;
	while(pos < count){
		sp = src + sstep*pos;
		val = sp[0] | (sp[1] << 8);
		val = (val & 0x7FFF) - (val & 0x8000);
		if(pos){
			sp -= sstep;
			last = sp[0] | (sp[1] << 8);
			last = (last & 0x7FFF) - (last & 0x8000);
		}
		out = last + (((val - last) * (phase & 0xFFFF)) >> 16);
		dp[0] = out;
		dp[1] = out >> 8;
		dp += dstep;
		phase += delta;
		pos = phase >> 16;
	}
	c->last = val;
	if(delta < 0x10000){
		c->phase = phase & 0xFFFF;
	} else {
		c->phase = phase - (count << 16);
	}
	return (dp - dst) / dstep;
}

static int
convertout(DSP *dsp, uchar *buf, int len, uchar **out)
{
	int ret, ch;
	ulong count, delta;

	/* no conversion required? */
	if(dsp->freq == dsp->rfreq && dsp->channels == CHANNELS){
		*out = buf;
		return len;
	}

	/*
	 * delta is the number of input samples to 
	 * produce one output sample. scaled by 16 bit to
	 * get fractional part.
	 */
	delta = ((ulong)dsp->freq << 16) / dsp->rfreq;
	count = len / (2 * dsp->channels);

	/*
	 * get maximum required size of output bufer. this is not exact!
	 * number of output samples depends on phase!
	 */
	ret = (((count << 16) + delta-1) / delta) * 2*CHANNELS;
	if(ret > dsp->nbuf){
		free(dsp->buf);
		dsp->buf = kmalloc(ret);
		dsp->nbuf = ret;
	}
	for(ch=0; ch < CHANNELS; ch++)
		ret = resample(dsp->chan + ch,
			buf + 2*(ch % dsp->channels),
			dsp->buf + 2*ch,
			2*dsp->channels,
			2*CHANNELS,
			delta,
			count);

	*out = dsp->buf;
	return ret * 2*CHANNELS;
}

static int
writedsp(Ufile *file, void *buf, int len, vlong)
{
	DSP *dsp = (DSP*)file;
	vlong now;
	int ret, diff;
	uchar *out;

	if((ret = convertout(dsp, buf, len, &out)) <= 0)
		return ret;

	if((ret = write(dsp->fd, out, ret)) < 0)
		return mkerror();

	now = nsec();
	if(dsp->time < now){
		dsp->time = now;
		dsp->written = 0;
		dsp->written2 = 0;
	} else {
		diff = (dsp->time - now) / 1000000;
		if(diff > DELAY)
			sleep(diff - DELAY);
	}
	dsp->time += ((1000000000LL) * ret / (dsp->rfreq * 2*CHANNELS));
	dsp->written += len;
	dsp->written2 += len;

	return len;
}

enum
{
	AFMT_S16_LE = 0x10,
};

static int
ioctldsp(Ufile *file, int cmd, void *arg)
{
	DSP *dsp = (DSP*)file;
	int ret, i;
	vlong now;
	static int counter;

	ret = 0;
	switch(cmd){
	default:
		trace("dsp: unknown ioctl %lux %p", (ulong)cmd, arg);
		ret = -ENOTTY;
		break;

	case 0xC004500A:
		trace("dsp: SNDCTL_DSP_SETFRAGMENT(%lux)", *(ulong*)arg);
		break;

	case 0xC0045004:
		trace("dsp: SNDCTL_DSP_GETBLKSIZE");
		*((int*)arg) = FRAGSIZE;
		break;

	case 0x800c5011:
		trace("dsp: SNDCTL_DSP_GETIPTR");
		ret = -EPERM;
		break;

	case 0x800c5012:
		trace("dsp: SNDCTL_DSP_GETOPTR");
		((int*)arg)[0] = dsp->written;				// Total # of bytes processed
		((int*)arg)[1] = dsp->written2 / FRAGSIZE;	// # of fragment transitions since last time
		dsp->written2 = 0;
		((int*)arg)[2] = 0;						// Current DMA pointer value
		break;

	case 0x8010500D:
		trace("dsp: SNDCTL_DSG_GETISPACE");
		ret = -EPERM;
		break;
	case 0x8010500C:
		trace("dsp: SNDCTL_DSP_GETOSPACE");
		i = (2 * dsp->channels) * ((dsp->freq*DELAY)/1000);
		((int*)arg)[1] = i / FRAGSIZE;				// fragstot
		((int*)arg)[2] = FRAGSIZE;					// fragsize
		now = nsec();
		if(now < dsp->time){
			i -= ((2 * dsp->channels) * (((dsp->time - now) * (vlong)dsp->freq) / 1000000000));
			if(i < 0)
				i = 0;
		}
		((int*)arg)[0] = i / FRAGSIZE;				// available fragment count
		((int*)arg)[3] = i;						// available space in bytes
		break;

	case 0x8004500B:
		trace("dsp: SNDCTL_DSP_GETFMTS(%d)", *(int*)arg);
		*(int*)arg = AFMT_S16_LE;
		break;

	case 0x8004500F:
		trace("dsp: SNDCTL_DSP_GETCAPS");
		*(int*)arg = 0x400;
		break;

	case 0xC0045005:
		trace("dsp: SNDCTL_DSP_SETFMT(%d)", *(int*)arg);
		*(int*)arg = AFMT_S16_LE;
		break;

	case 0xC0045006:
		trace("dsp: SOUND_PCM_WRITE_CHANNELS(%d)", *(int*)arg);
		dsp->channels = *(int*)arg;
		break;

	case 0xC0045003:
		trace("dsp: SNDCTL_DSP_STEREO(%d)", *(int*)arg);
		dsp->channels = 2;
		*(int*)arg = 1;
		break;

	case 0xC0045002:
		trace("dsp: SNDCTL_DSP_SPEED(%d)", *(int*)arg);
		dsp->freq = *(int*)arg;
		for(i=0; i<CHANNELS; i++){
			dsp->chan[i].phase = 0;
			dsp->chan[i].last = 0;
		}
		break;

	case 0x00005000:
		trace("dsp: SNDCTL_DSP_RESET");
		break;

	case 0x00005001:
		trace("dsp: SNDCTL_DSP_SYNC");
		break;
	}

	return ret;
}

static int
getaudiofreq(void)
{
	int ret, n, fd;
	char buf[1024];

	ret = FREQUENCY;
	if((fd = open("/dev/volume", OREAD)) < 0)
		return ret;
	if((n = read(fd, buf, sizeof(buf)-1)) > 0){
		char *p;

		buf[n] = 0;
		if(p = strstr(buf, "speed out "))
			ret = atoi(p + 10);
	}
	close(fd);
	return ret;
}

int opendsp(char *path, int mode, int, Ufile **pf)
{
	DSP *dsp;
	int freq;
	int fd;

	if(strcmp(path, "/dev/dsp")==0 || strcmp(path, "/dev/dsp0")==0){
		if((fd = open("/dev/audio", OWRITE)) < 0)
			return mkerror();

		freq = getaudiofreq();
		dsp = mallocz(sizeof(DSP), 1);
		dsp->ref = 1;
		dsp->mode = mode;
		dsp->dev = DSPDEV;
		dsp->fd = fd;
		dsp->path = kstrdup(path);
		dsp->rfreq = freq;
		dsp->freq = freq;
		dsp->channels = CHANNELS;

		*pf = dsp;
		return 0;
	}

	return -ENOENT;
}

static int
fstatdsp(Ufile *f, Ustat *s)
{
	s->mode = 0666 | S_IFCHR;
	s->uid = current->uid;
	s->gid = current->gid;
	s->ino = hashpath(f->path);
	s->size = 0;
	return 0;
};

static int
statdsp(char *path, int , Ustat *s)
{
	if(strcmp(path, "/dev/dsp")==0 || strcmp(path, "/dev/dsp0")==0){
		s->mode = 0666 | S_IFCHR;
		s->uid = current->uid;
		s->gid = current->gid;
		s->ino = hashpath(path);
		s->size = 0;
		return 0;
	}

	return -ENOENT;
}

static Udev dspdev = 
{
	.open = opendsp,
	.read = readdsp,
	.write = writedsp,
	.poll = polldsp,
	.close = closedsp,
	.ioctl = ioctldsp,
	.stat = statdsp,
	.fstat = fstatdsp,
};

void dspdevinit(void)
{
	devtab[DSPDEV] = &dspdev;

	fsmount(&dspdev, "/dev/dsp");
	fsmount(&dspdev, "/dev/dsp0");
}
