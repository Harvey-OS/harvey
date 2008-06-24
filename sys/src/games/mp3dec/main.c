/*
 * Simple mp3 player.  Derived from libmad's example minimad.c.
 */
#include <u.h>
#include <libc.h>
#include "mad.h"

/* Current input file */
char *name;
vlong offset;
int rate = 44100;

char *outfile;
int vfd; /* /dev/volume */

static enum mad_flow
input(void *data, struct mad_stream *stream)
{
	int fd, n, m;
	static uchar buf[32768];

	fd = (int)data;
	n = stream->bufend - stream->next_frame;
	memmove(buf, stream->next_frame, n);
	m = read(fd, buf+n, sizeof buf-n);
	offset += m;
	if(m < 0)
		sysfatal("reading input: %r");
	if(m == 0)
		return MAD_FLOW_STOP;
	n += m;
	mad_stream_buffer(stream, buf, n);
	return MAD_FLOW_CONTINUE;
}

/*
 * Dither 28-bit down to 16-bit.  From mpg321. 
 * I'm skeptical, but it's supposed to make the
 * samples sound better than just truncation.
 */
typedef struct Dither Dither;
struct Dither
{
	mad_fixed_t error[3];
	mad_fixed_t random;
};

#define PRNG(x) (((x)*0x19660dL + 0x3c6ef35fL) & 0xffffffffL)

enum
{
	FracBits = MAD_F_FRACBITS,
	OutBits = 16,
	Round = 1 << (FracBits+1-OutBits-1),	// sic
	ScaleBits = FracBits + 1 - OutBits,
	LowMask  = (1<<ScaleBits) - 1,
	Min = -MAD_F_ONE,
	Max = MAD_F_ONE - 1,
};

int
audiodither(mad_fixed_t v, Dither *d)
{
	int out;
	mad_fixed_t random;

	/* noise shape */
	v += d->error[0] - d->error[1] + d->error[2];
	d->error[2] = d->error[1];
	d->error[1] = d->error[0] / 2;
	
	/* bias */
	out = v + Round;
	
	/* dither */
	random = PRNG(d->random);
	out += (random & LowMask) - (d->random & LowMask);
	d->random = random;
	
	/* clip */
	if(out > Max){
		out = Max;
		if(v > Max)
			v = Max;
	}else if(out < Min){
		out = Min;
		if(v < Min)
			v = Min;
	}
	
	/* quantize */
	out &= ~LowMask;
	
	/* error feedback */
	d->error[0] = v - out;
	
	/* scale */
	return out >> ScaleBits;
}

static enum mad_flow
output(void *data, struct mad_header const* header, struct mad_pcm *pcm)
{
	int i, n, v;
	mad_fixed_t const *left, *right;
	static Dither d;
	static uchar buf[16384], *p;

	if(pcm->samplerate != rate){
		rate = pcm->samplerate;
		if(vfd < 0)
			fprint(2, "warning: audio sample rate is %d Hz\n", rate);
		else
			fprint(vfd, "speed %d", rate);
	}
	p = buf;
	memset(&d, 0, sizeof d);
	n = pcm->length;
	switch(pcm->channels){
	case 1:
		left = pcm->samples[0];
		for(i=0; i<n; i++){
			v = audiodither(*left++, &d);
			/* stereoize */
			*p++ = v;
			*p++ = v>>8;
			*p++ = v;
			*p++ = v>>8;
		}
		break;
	case 2:
		left = pcm->samples[0];
		right = pcm->samples[1];
		for(i=0; i<n; i++){
			v = audiodither(*left++, &d);
			*p++ = v;
			*p++ = v>>8;
			v = audiodither(*right++, &d);
			*p++ = v;
			*p++ = v>>8;
		}
		break;
	}
	assert(p<=buf+sizeof buf);
	write(1, buf, p-buf);
	return MAD_FLOW_CONTINUE;
}

static enum mad_flow
error(void *data, struct mad_stream *stream, struct mad_frame *frame)
{
	if(stream->error == MAD_ERROR_LOSTSYNC)
		return MAD_FLOW_CONTINUE;
	sysfatal("%s:#%lld: %s",
		name, offset-(stream->bufend-stream->next_frame),
		mad_stream_errorstr(stream));
	return 0;
}

void
play(int fd, char *nam)
{
	struct mad_decoder decoder;
	
	name = nam;
	mad_decoder_init(&decoder, (void*)fd, input, nil, nil, output, error, nil);
	mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
	mad_decoder_finish(&decoder);
}

void
usage(void)
{
	fprint(2, "usage: mp3dec [-o outfile] [file...]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i, fd;
	char *p;

	ARGBEGIN{
	case 'o':
		outfile = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND
	
	if(outfile){
		if((fd = create(outfile, OWRITE, 0666)) < 0)
			sysfatal("create %s: %r", outfile);
	}else{
		if((fd = open("/dev/audio", OWRITE)) < 0)
			sysfatal("open /dev/audio: %r");
		vfd = open("/dev/volume", OWRITE);
	}
	if(fd != 1){
		dup(fd, 1);
		close(fd);
	}

	if(argc == 0){
		play(0, "standard input");
	}else{
		for(i=0; i<argc; i++){
			if((fd = open(argv[i], OREAD)) < 0)
				sysfatal("open %s: %r", argv[i]);
			play(fd, argv[i]);
			close(fd);
		}
	}
	exits(0);
}
