#include	<alef.h>
#include	<bio.h>

void*
Biobufhdr.rdline(Biobufhdr *bp, int delim)
{
	char *ip, *ep;
	int i, j;

	bp->lock();
	i = -bp->icount;
	if(i == 0) {
		/*
		 * eof or other error
		 */
		if(bp->state != Bractive) {
			bp->nrdline = 0;
			bp->unlock();
			return nil;
		}
	}

	/*
	 * first try in remainder of buffer
	 */
	ip = (char*)bp->ebuf - i;
	ep = memchr(ip, delim, i);
	if(ep) {
		j = (ep - ip) + 1;
		bp->nrdline = j;
		bp->icount += j;
		bp->unlock();
		return ip;
	}

	/*
	 * copy data to beginning of buffer
	 */
	if(i < bp->bsize)
		memmove(bp->bbuf, ip, i);

	/*
	 * append to buffer looking for the delim
	 */
	ip = (char*)bp->bbuf + i;
	while(i < bp->bsize) {
		j = read(bp->fid, ip, bp->bsize-i);
		if(j <= 0) {
			/*
			 * end of file with no delim
			 */
			memmove(bp->ebuf-i, bp->bbuf, i);
			bp->nrdline = i;
			bp->icount = -i;
			bp->unlock();
			return nil;
		}
		bp->off += j;
		i += j;
		ep = memchr(ip, delim, j);
		if(ep) {
			/*
			 * found in new piece
			 * copy back up and reset everything
			 */
			ip = (char*)bp->ebuf - i;
			if(i < bp->bsize)
				memmove(ip, bp->bbuf, i);
			j = (ep - (char*)bp->bbuf) + 1;
			bp->nrdline = j;
			bp->icount = j - i;
			bp->unlock();
			return ip;
		}
		ip += j;
	}

	/*
	 * full buffer without finding
	 */
	bp->nrdline = bp->bsize;
	bp->icount = -bp->bsize;
	bp->unlock();
	return nil;
}

int
Biobufhdr.linelen(Biobufhdr *bp)
{
	return bp->nrdline;
}
