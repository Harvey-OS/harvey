#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>

Memsubfont*
openmemsubfont(char *name)
{
	Memsubfont *sf;
	Memimage *i;
	Fontchar *fc;
	int fd, n;
	char hdr[3*12+4+1];
	uchar *p;

	fd = open(name, OREAD);
	if(fd < 0)
		return nil;
	i = readmemimage(fd);
	if(i == nil)
		return nil;
	if(read(fd, hdr, 3*12) != 3*12){
		freememimage(i);
		werrstr("openmemsubfont: header read error: %r");
		return nil;
	}
	n = atoi(hdr);
	p = malloc(6*(n+1));
	if(p == nil){
		freememimage(i);
		return nil;
	}
	if(read(fd, p, 6*(n+1)) != 6*(n+1)){
		werrstr("openmemsubfont: fontchar read error: %r");
    Err:
		freememimage(i);
		free(p);
		return nil;
	}
	fc = malloc(sizeof(Fontchar)*(n+1));
	if(fc == nil)
		goto Err;
	_unpackinfo(fc, p, n);
	sf = allocmemsubfont(name, n, atoi(hdr+12), atoi(hdr+24), fc, i);
	if(sf == nil){
		free(fc);
		goto Err;
	}
	free(p);
	return sf;
}
