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
#include <auth.h>
#include <mp.h>
#include <libsec.h>

static char*
readfile(char *name)
{
	int fd;
	char *s;
	Dir *d;

	fd = open(name, OREAD);
	if(fd < 0)
		return nil;
	if((d = dirfstat(fd)) == nil) {
		close(fd);
		return nil;
	}
	s = malloc(d->length + 1);
	if(s == nil || readn(fd, s, d->length) != d->length){
		free(s);
		free(d);
		close(fd);
		return nil;
	}
	close(fd);
	s[d->length] = '\0';
	free(d);
	return s;
}

uint8_t*
readcert(char *filename, int *pcertlen)
{
	char *pem;
	uint8_t *binary;

	pem = readfile(filename);
	if(pem == nil){
		werrstr("can't read %s: %r", filename);
		return nil;
	}
	binary = decodePEM(pem, "CERTIFICATE", pcertlen, nil);
	free(pem);
	if(binary == nil){
		werrstr("can't parse %s", filename);
		return nil;
	}
	return binary;
}

PEMChain *
readcertchain(char *filename)
{
	char *chfile;

	chfile = readfile(filename);
	if (chfile == nil) {
		werrstr("can't read %s: %r", filename);
		return nil;
	}
	return decodepemchain(chfile, "CERTIFICATE");
}
