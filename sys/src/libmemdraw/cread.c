/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>

Memimage*
creadmemimage(int fd)
{
	char hdr[5*12+1];
	Rectangle r;
	int m, nb, miny, maxy, new, ldepth, ncblock;
	u8 *buf;
	Memimage *i;
	u32 chan;

	if(readn(fd, hdr, 5*12) != 5*12){
		werrstr("readmemimage: short header (2)");
		return nil;
	}

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

	i = allocmemimage(r, chan);
	if(i == nil)
		return nil;
	ncblock = _compblocksize(r, i->depth);
	buf = malloc(ncblock);
	if(buf == nil)
		goto Errout;
	miny = r.min.y;
	while(miny != r.max.y){
		if(readn(fd, hdr, 2*12) != 2*12){
		Shortread:
			werrstr("readmemimage: short read");
		Errout:
			freememimage(i);
			free(buf);
			return nil;
		}
		maxy = atoi(hdr+0*12);
		nb = atoi(hdr+1*12);
		if(maxy<=miny || r.max.y<maxy){
			werrstr("readimage: bad maxy %d", maxy);
			goto Errout;
		}
		if(nb<=0 || ncblock<nb){
			werrstr("readimage: bad count %d", nb);
			goto Errout;
		}
		if(readn(fd, buf, nb)!=nb)
			goto Shortread;
		if(!new)	/* old image: flip the data bits */
			_twiddlecompressed(buf, nb);
		cloadmemimage(i, Rect(r.min.x, miny, r.max.x, maxy), buf, nb);
		miny = maxy;
	}
	free(buf);
	return i;
}
