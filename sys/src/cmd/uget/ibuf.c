#include "mothra.h"


Ibuf*
ibufalloc(int fd)
{
	Ibuf* b;

	b = emalloc(sizeof(*b));
	b->fd = fd;
	b->rp = b->wp = b->buf;
	b->ep = b->rp + sizeof(b->buf);
	return b;
}

Ibuf*
ibufopen(char *file)
{
	int fd;

	fd = open(file, OREAD);
	if(fd < 0)
		return 0;
	return ibufalloc(fd);
}

void
ibufclose(Ibuf *b)
{
	close(b->fd);
	free(b);
}

char*
rdline(Ibuf *b)
{
	char *p;
	int n;

	for(;;){
		for(p = b->rp; p < b->wp; p++)
			if(*p == '\n'){
				b->lp = b->rp;
				b->rp = p+1;
				return b->lp;
			}
		n = b->wp - b->rp;
		if(n >= 0)
			memmove(b->buf, b->rp, n);
		b->rp = b->buf;
		b->wp = b->rp + n;
		n = b->ep - b->wp;
		if(n <= 0){
			b->lp = b->rp;
			b->rp = b->wp;
			return b->lp;
		}
		n = read(b->fd, b->wp, n);
		if(n <= 0)
			return 0;
		b->wp += n;
	}
	/* unreachable */
	return 0;
}

int
linelen(Ibuf *b)
{
	return b->rp - b->lp;
}
