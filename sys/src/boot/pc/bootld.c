#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

static int
addbytes(char **dbuf, char *edbuf, char **sbuf, char *esbuf)
{
	int n;

	n = edbuf - *dbuf;
	if(n <= 0)
		return 0;
	if(n > esbuf - *sbuf)
		n = esbuf - *sbuf;
	if(n <= 0)
		return -1;

	memmove(*dbuf, *sbuf, n);
	*sbuf += n;
	*dbuf += n;
	return edbuf - *dbuf;
}

extern void origin(void);

int
bootpass(Boot *b, void *vbuf, int nbuf)
{
	char *buf, *ebuf, *p, *q;
	ulong size;

	if(b->state == FAILED)
		return FAIL;

	if(nbuf == 0)
		goto Endofinput;

	buf = vbuf;
	ebuf = buf+nbuf;
	while(addbytes(&b->wp, b->ep, &buf, ebuf) == 0) {
		switch(b->state) {
		case INIT9LOAD:
			b->state = READ9LOAD;
			b->bp = (char*)0x10000;
			b->wp = b->bp;
			b->ep = b->bp + 256*1024;
			break;

		case READ9LOAD:
			return ENOUGH;

		default:
			panic("bootstate");
		}
	}
	return MORE;


Endofinput:
	/* end of input */
	print("\n");
	switch(b->state) {
	case INIT9LOAD:
		print("premature EOF\n");
		b->state = FAILED;
		return FAIL;
	
	case READ9LOAD:
		size = b->wp - b->bp;
		if(memcmp(b->bp, origin, 16) != 0) {
			print("beginning of file does not look right\n");
			b->state = FAILED;
			return FAIL;
		}
		if(size < 32*1024 || size > 256*1024) {
			print("got %lud byte loader; not likely\n", size);
			b->state = FAILED;
			return FAIL;
		}

		p = b->bp;
		q = b->wp;
		if(q - p > 10000)	/* don't search much past l.s */
			q = p+10000;

		/*
		 * The string `JUMP' appears right before
		 * tokzero, which is where we want to jump.
		 */
		for(; p<q; p++) {
			if(strncmp(p, "JUMP", 4) == 0) {
				p += 4;
				warp9((ulong)p);
			}
		}
		print("could not find jump destination\n");
		b->state = FAILED;
		return FAIL;

	default:
		panic("bootdone");
	}
	b->state = FAILED;
	return FAIL;
}
