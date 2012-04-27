/*
 * Linux and BSD
 */
#include <sys/ioctl.h>
#ifdef __linux__
#include <linux/soundcard.h>
#else
#include <sys/soundcard.h>
#endif
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
	Bigendian = 1,
};

static int afd = -1;
static int cfd= -1;
static int speed;

/* maybe this should return -1 instead of sysfatal */
void
audiodevopen(void)
{
	int t;
	ulong ul;

	afd = -1;
	cfd = -1;
	if((afd = open("/dev/dsp", OWRITE)) < 0)
		goto err;
	if((cfd = open("/dev/mixer", ORDWR)) < 0)
		goto err;
	
	t = Bits;
	if(ioctl(afd, SNDCTL_DSP_SAMPLESIZE, &t) < 0)
		goto err;
	
	t = Channels-1;
	if(ioctl(afd, SNDCTL_DSP_STEREO, &t) < 0)
		goto err;
	
	speed = Rate;
	ul = Rate;
	if(ioctl(afd, SNDCTL_DSP_SPEED, &ul) < 0)
		goto err;

	return;

err:
	if(afd >= 0)
		close(afd);
	afd = -1;
	oserror();
}

void
audiodevclose(void)
{
	close(afd);
	close(cfd);
	afd = -1;
	cfd = -1;
}

static struct {
	int id9;
	int id;
} names[] = {
	Vaudio,	SOUND_MIXER_VOLUME,
	Vbass, 		SOUND_MIXER_BASS,
	Vtreb, 		SOUND_MIXER_TREBLE,
	Vline, 		SOUND_MIXER_LINE,
	Vpcm, 		SOUND_MIXER_PCM,
	Vsynth, 		SOUND_MIXER_SYNTH,
	Vcd, 		SOUND_MIXER_CD,
	Vmic, 		SOUND_MIXER_MIC,
//	"record", 		SOUND_MIXER_RECLEV,
//	"mix",		SOUND_MIXER_IMIX,
//	"pcm2",		SOUND_MIXER_ALTPCM,
	Vspeaker,	SOUND_MIXER_SPEAKER
//	"line1",		SOUND_MIXER_LINE1,
//	"line2",		SOUND_MIXER_LINE2,
//	"line3",		SOUND_MIXER_LINE3,
//	"digital1",	SOUND_MIXER_DIGITAL1,
//	"digital2",	SOUND_MIXER_DIGITAL2,
//	"digital3",	SOUND_MIXER_DIGITAL3,
//	"phonein",		SOUND_MIXER_PHONEIN,
//	"phoneout",		SOUND_MIXER_PHONEOUT,
//	"radio",		SOUND_MIXER_RADIO,
//	"video",		SOUND_MIXER_VIDEO,
//	"monitor",	SOUND_MIXER_MONITOR,
//	"igain",		SOUND_MIXER_IGAIN,
//	"ogain",		SOUND_MIXER_OGAIN,
};

static int
lookname(int id9)
{
	int i;
	
	for(i=0; i<nelem(names); i++)
		if(names[i].id9 == id9)
			return names[i].id;
	return -1;
}

void
audiodevsetvol(int what, int left, int right)
{
	int id;
	ulong x;
	int can, v;
	
	if(cfd < 0)
		error("audio device not open");
	if(what == Vspeed){
		x = left;
		if(ioctl(afd, SNDCTL_DSP_SPEED, &x) < 0)
			oserror();
		speed = x;
		return;
	}
	if((id = lookname(what)) < 0)
		return;
	if(ioctl(cfd, SOUND_MIXER_READ_DEVMASK, &can) < 0)
		can = ~0;
	if(!(can & (1<<id)))
		return;
	v = left | (right<<8);
	if(ioctl(cfd, MIXER_WRITE(id), &v) < 0)
		oserror();
}

void
audiodevgetvol(int what, int *left, int *right)
{
	int id;
	int can, v;
	
	if(cfd < 0)
		error("audio device not open");
	if(what == Vspeed){
		*left = *right = speed;
		return;
	}
	if((id = lookname(what)) < 0)
		return;
	if(ioctl(cfd, SOUND_MIXER_READ_DEVMASK, &can) < 0)
		can = ~0;
	if(!(can & (1<<id)))
		return;
	if(ioctl(cfd, MIXER_READ(id), &v) < 0)
		oserror();
	*left = v&0xFF;
	*right = (v>>8)&0xFF;
}

int
audiodevwrite(void *v, int n)
{
	int m, tot;
	
	for(tot=0; tot<n; tot+=m)
		if((m = write(afd, (uchar*)v+tot, n-tot)) <= 0)
			oserror();
	return tot;
}

int
audiodevread(void *v, int n)
{
	error("no reading");
	return -1;
}
