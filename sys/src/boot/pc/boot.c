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

int
bootpass(Boot *b, void *vbuf, int nbuf)
{
	char *buf, *ebuf;
	Exec *ep;
	ulong entry, data, text, bss;

	if(b->state == FAILED)
		return FAIL;

	if(nbuf == 0)
		goto Endofinput;

	buf = vbuf;
	ebuf = buf+nbuf;
	while(addbytes(&b->wp, b->ep, &buf, ebuf) == 0) {
		switch(b->state) {
		case INITKERNEL:
			b->state = READEXEC;
			b->bp = (char*)&b->exec;
			b->wp = b->bp;
			b->ep = b->bp+sizeof(Exec);
			break;
		case READEXEC:
			ep = &b->exec;
			if(GLLONG(ep->magic) == I_MAGIC) {
				b->state = READTEXT;
				b->bp = (char*)PADDR(GLLONG(ep->entry));
				b->wp = b->bp;
				b->ep = b->wp+GLLONG(ep->text);
				print("%lud", GLLONG(ep->text));
				break;
			}

			/* check for gzipped kernel */
			if(b->bp[0] == 0x1F && (uchar)b->bp[1] == 0x8B && b->bp[2] == 0x08) {
				b->state = READGZIP;
				b->bp = (char*)malloc(1440*1024);
				b->wp = b->bp;
				b->ep = b->wp + 1440*1024;
				memmove(b->bp, &b->exec, sizeof(Exec));
				b->wp += sizeof(Exec);
				print("gz...");
				break;
			}

			print("bad kernel format\n");
			b->state = FAILED;
			return FAIL;

		case READTEXT:
			ep = &b->exec;
			b->state = READDATA;
			b->bp = (char*)PGROUND(GLLONG(ep->entry)+GLLONG(ep->text));
			b->wp = b->bp;
			b->ep = b->wp + GLLONG(ep->data);
			print("+%ld", GLLONG(ep->data));
			break;
	
		case READDATA:
			ep = &b->exec;
			bss = GLLONG(ep->bss);
			print("+%ld=%ld\n",
				bss, GLLONG(ep->text)+GLLONG(ep->data)+bss);
			b->state = TRYBOOT;
			return ENOUGH;

		case TRYBOOT:
		case READGZIP:
			return ENOUGH;

		case READ9LOAD:
		case INIT9LOAD:
			panic("9load");

		default:
			panic("bootstate");
		}
	}
	return MORE;


Endofinput:
	/* end of input */
	switch(b->state) {
	case INITKERNEL:
	case READEXEC:
	case READDATA:
	case READTEXT:
		print("premature EOF\n");
		b->state = FAILED;
		return FAIL;
	
	case TRYBOOT:
		entry = GLLONG(b->exec.entry);
		print("entry: 0x%lux\n", entry);
		warp9(PADDR(entry));
		b->state = FAILED;
		return FAIL;

	case READGZIP:
		ep = &b->exec;
		if(b->bp[0] != 0x1F || (uchar)b->bp[1] != 0x8B || b->bp[2] != 0x08)
			print("lost magic\n");

		print("%ld => ", b->wp - b->bp);
		if(gunzip((uchar*)ep, sizeof(*ep), (uchar*)b->bp, b->wp - b->bp) < sizeof(*ep)) {
			print("badly compressed kernel\n");
			return FAIL;
		}
		entry = GLLONG(ep->entry);
		text = GLLONG(ep->text);
		data = GLLONG(ep->data);
		bss = GLLONG(ep->bss);
		print("%lud+%lud+%lud=%lud\n", text, data, bss, text+data+bss);

		if(gunzip((uchar*)PADDR(entry)-sizeof(Exec), sizeof(Exec)+text+data, 
		     (uchar*)b->bp, b->wp-b->bp) < sizeof(Exec)+text+data) {
			print("error uncompressing kernel\n");
			return FAIL;
		}

		/* relocate data to start at page boundary */
		memmove((void*)PGROUND(entry+text), (void*)(entry+text), data);

		print("entry: %lux\n", entry);
		warp9(PADDR(entry));
		b->state = FAILED;
		return FAIL;

	case INIT9LOAD:
	case READ9LOAD:
		panic("end 9load");

	default:
		panic("bootdone");
	}
	b->state = FAILED;
	return FAIL;
}
