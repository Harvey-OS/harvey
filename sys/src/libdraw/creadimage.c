#include <u.h>
#include <libc.h>
#include <draw.h>

/*
 * compressed data are sequences of byte codes.  
 * if the first byte b has the 0x80 bit set, the next (b^0x80)+1 bytes
 * are data.  otherwise, it's two bytes specifying a previous string to repeat.
 */
static void
twiddlecompressed(uchar *buf, int n)
{
	uchar *ebuf;
	int j, k, c;

	ebuf = buf+n;
	while(buf < ebuf){
		c = *buf++;
		if(c >= 128){
			k = c-128+1;
			for(j=0; j<k; j++, buf++)
				*buf ^= 0xFF;
		}else
			buf++;
	}
}

Image *
creadimage(Display *d, int fd, int dolock)
{
	char hdr[5*12+1];
	Rectangle r;
	int m, nb, miny, maxy, new, ldepth;
	uchar *buf, *a;
	Image *i;
	ulong chan;

	if(readn(fd, hdr, 5*12) != 5*12)
		return nil;

	/*
	 * distinguish new channel descriptor from old ldepth.
	 * channel descriptors have letters as well as numbers,
	 * while ldepths are a single digit formatted as %-11d.
	 */
	new = 0;
	for(m=0; m<10; m++){
		if(hdr[m] != ' '){
			new = 1;
			break;
		}
	}
	if(hdr[11] != ' '){
		werrstr("creadimage: bad format");
		return nil;
	}
	if(new){
		hdr[11] = '\0';
		if((chan = strtochan(hdr)) == 0){
			werrstr("creadimage: bad channel string %s", hdr);
			return nil;
		}
	}else{
		ldepth = ((int)hdr[10])-'0';
		if(ldepth<0 || ldepth>3){
			werrstr("creadimage: bad ldepth %d", ldepth);
			return nil;
		}
		chan = drawld2chan[ldepth];
	}
	r.min.x=atoi(hdr+1*12);
	r.min.y=atoi(hdr+2*12);
	r.max.x=atoi(hdr+3*12);
	r.max.y=atoi(hdr+4*12);
	if(r.min.x>r.max.x || r.min.y>r.max.y){
		werrstr("creadimage: bad rectangle");
		return nil;
	}

	if(dolock)
		lockdisplay(d);
	i = allocimage(d, r, chan, 0, 0);
	if(dolock)
		unlockdisplay(d);
	if(i == nil)
		return nil;
	buf = malloc(NCBLOCK);
	if(buf == nil)
		goto Errout;
	miny = r.min.y;
	while(miny != r.max.y){
		if(readn(fd, hdr, 2*12) != 2*12){
		Errout:
			if(dolock)
				lockdisplay(d);
		Erroutlock:
			freeimage(i);
			if(dolock)
				unlockdisplay(d);
			free(buf);
			return nil;
		}
		maxy = atoi(hdr+0*12);
		nb = atoi(hdr+1*12);
		if(maxy<=miny || r.max.y<maxy){
			werrstr("creadimage: bad maxy %d", maxy);
			goto Errout;
		}
		if(nb<=0 || NCBLOCK<nb){
			werrstr("creadimage: bad count %d", nb);
			goto Errout;
		}
		if(readn(fd, buf, nb)!=nb)
			goto Errout;
		if(dolock)
			lockdisplay(d);
		a = bufimage(i->display, 21+nb);
		if(a == nil)
			goto Erroutlock;
		a[0] = 'Y';
		BPLONG(a+1, i->id);
		BPLONG(a+5, r.min.x);
		BPLONG(a+9, miny);
		BPLONG(a+13, r.max.x);
		BPLONG(a+17, maxy);
		if(!new)	/* old image: flip the data bits */
			twiddlecompressed(buf, nb);
		memmove(a+21, buf, nb);
		if(dolock)
			unlockdisplay(d);
		miny = maxy;
	}
	free(buf);
	return i;
}
