/*
 * Sun
 */
#include <sys/ioctl.h>
#include <sys/audio.h>
#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"
#include	"devaudio.h"

enum
{
	Channels = 2,
	Rate = 44100,
	Bits = 16,
};

static char* afn = 0;
static char* cfn = 0;
static int afd = -1;
static int cfd = -1;
static int speed = Rate;
static int needswap = -1;

static void
audiodevinit(void)
{
	uchar *p;
	ushort leorder;

	if ((afn = getenv("AUDIODEV")) == nil)
		afn = "/dev/audio";
	cfn = (char*)malloc(strlen(afn) + 3 + 1);
	if(cfn == nil)
		panic("out of memory");
	strcpy(cfn, afn);
	strcat(cfn, "ctl");

	/*
	 * Plan 9 /dev/audio is always little endian;
	 * solaris /dev/audio seems to expect native byte order,
	 * so on big endian machine (like sparc) we have to swap.
	 */
	leorder = (ushort) 0x0100;
	p = (uchar*)&leorder;
	if (p[0] == 0 && p[1] == 1) {
		/* little-endian: nothing to do */
		needswap = 0;
	} else {
		/* big-endian: translate Plan 9 little-endian */
		needswap = 1;
	}
}

/* maybe this should return -1 instead of sysfatal */
void
audiodevopen(void)
{
	audio_info_t info;
	struct audio_device ad;

	if (afn == nil || cfn == nil)
		audiodevinit();
	if((afd = open(afn, O_WRONLY)) < 0)
		goto err;
	if(cfd < 0 && (cfd = open(cfn, O_RDWR)) < 0)
		goto err;

	AUDIO_INITINFO(&info);
	info.play.precision = Bits;
	info.play.channels = Channels;
	info.play.sample_rate = speed;
	info.play.encoding = AUDIO_ENCODING_LINEAR;
	if(ioctl(afd, AUDIO_SETINFO, &info) < 0)
		goto err;

	return;

err:
	if(afd >= 0)
		close(afd);
	afd = -1;
	if(cfd >= 0)
		close(cfd);
	cfd = -1;
	oserror();
}

void
audiodevclose(void)
{
	if(afd >= 0)
		close(afd);
	if(cfd >= 0)
		close(cfd);
	afd = -1;
	cfd = -1;
}

static double
fromsun(double val, double min, double max)
{
	return (val-min) / (max-min);
}

static double
tosun(double val, double min, double max)
{
	return val*(max-min) + min;
}

static void
setvolbal(double left, double right)
{
	audio_info_t info;
	double vol, bal;

	if (left < 0 || right < 0) {
		/* should not happen */
		return;
	} else if (left == right) {
		vol = tosun(left/100.0, AUDIO_MIN_GAIN, AUDIO_MAX_GAIN);
		bal = AUDIO_MID_BALANCE;
	} else if (left < right) {
		vol = tosun(right/100.0, AUDIO_MIN_GAIN, AUDIO_MAX_GAIN);
		bal = tosun(1.0 - left/right, AUDIO_MID_BALANCE, AUDIO_RIGHT_BALANCE);
	} else if (right < left) {
		vol = tosun(left/100.0, AUDIO_MIN_GAIN, AUDIO_MAX_GAIN);
		bal = tosun(1.0 - right/left, AUDIO_MID_BALANCE, AUDIO_LEFT_BALANCE);
	}
	AUDIO_INITINFO(&info);
	info.play.gain = (long)(vol+0.5);
	info.play.balance = (long)(bal+0.5);
	if(ioctl(cfd, AUDIO_SETINFO, &info) < 0)
		oserror();
}

static void
getvolbal(int *left, int *right)
{
	audio_info_t info;
	double gain, bal, vol, l, r;

	AUDIO_INITINFO(&info);
	if (ioctl(cfd, AUDIO_GETINFO, &info) < 0)
		oserror();

	gain = info.play.gain;
	bal = info.play.balance;
	vol = fromsun(gain, AUDIO_MIN_GAIN, AUDIO_MAX_GAIN) * 100.0;

	if (bal == AUDIO_MID_BALANCE) {
		l = r = vol;
	} else if (bal < AUDIO_MID_BALANCE) {
		l = vol;
		r = vol * (1.0 - fromsun(bal, AUDIO_MID_BALANCE, AUDIO_LEFT_BALANCE));
	} else {
		r = vol;
		l = vol * (1.0 - fromsun(bal, AUDIO_MID_BALANCE, AUDIO_RIGHT_BALANCE));
	}
	*left = (long)(l+0.5);
	*right = (long)(r+0.5);
	return;
}

void
audiodevsetvol(int what, int left, int right)
{
	audio_info_t info;
	ulong x;
	int l, r;
	
	if (afn == nil || cfn == nil)
		audiodevinit();
	if(cfd < 0 && (cfd = open(cfn, O_RDWR)) < 0) {
		cfd = -1;
		oserror();
	}

	if(what == Vspeed){
		x = left;
		AUDIO_INITINFO(&info);
		info.play.sample_rate = x;
		if(ioctl(cfd, AUDIO_SETINFO, &info) < 0)
			oserror();
		speed = x;
		return;
	}
	if(what == Vaudio){
		getvolbal(&l, &r);
		if (left < 0)
			setvolbal(l, right);
		else if (right < 0)
			setvolbal(left, r);
		else 
			setvolbal(left, right);
		return;
	}
}

void
audiodevgetvol(int what, int *left, int *right)
{
	audio_info_t info;

	if (afn == nil || cfn == nil)
		audiodevinit();
	if(cfd < 0 && (cfd = open(cfn, O_RDWR)) < 0) {
		cfd = -1;
		oserror();
	}
	switch(what) {
	case Vspeed:
		*left = *right = speed;
		break;
	case Vaudio:
		getvolbal(left, right);
		break;
	case Vtreb:
	case Vbass:
		*left = *right = 50;
		break;
	default:
		*left = *right = 0;
	}
}


static uchar *buf = 0;
static int nbuf = 0;

int
audiodevwrite(void *v, int n)
{
	int i, m, tot;
	uchar *p;

	if (needswap) {
		if (nbuf < n) {
			buf = (uchar*)erealloc(buf, n);
			if(buf == nil)
				panic("out of memory");
			nbuf = n;
		}

		p = (uchar*)v;
		for(i=0; i+1<n; i+=2) {
			buf[i] = p[i+1];
			buf[i+1] = p[i];
		}
		p = buf;
	} else
		p = (uchar*)v;
	
	for(tot=0; tot<n; tot+=m)
		if((m = write(afd, p+tot, n-tot)) <= 0)
			oserror();
	return tot;
}

int
audiodevread(void *v, int n)
{
	error("no reading");
	return -1;
}
