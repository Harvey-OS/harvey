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
#include <mp.h>
#include <libsec.h>

#define STRLEN(s)	(sizeof(s)-1)

u8 *
decodePEM(char *s, char *type, int *len, char **new_s)
{
	u8 *d;
	char *t, *e, *tt;
	int n;

	*len = 0;

	/*
	 * find the correct section of the file, stripping garbage at the beginning and end.
	 * the data is delimited by -----BEGIN <type>-----\n and -----END <type>-----\n
	 */
	n = strlen(type);
	e = strchr(s, '\0');
	for(t = s; t != nil && t < e; ){
		tt = t;
		t = strchr(tt, '\n');
		if(t != nil)
			t++;
		if(strncmp(tt, "-----BEGIN ", STRLEN("-----BEGIN ")) == 0
		&& strncmp(&tt[STRLEN("-----BEGIN ")], type, n) == 0
		&& strncmp(&tt[STRLEN("-----BEGIN ")+n], "-----\n", STRLEN("-----\n")) == 0)
			break;
	}
	for(tt = t; tt != nil && tt < e; tt++){
		if(strncmp(tt, "-----END ", STRLEN("-----END ")) == 0
		&& strncmp(&tt[STRLEN("-----END ")], type, n) == 0
		&& strncmp(&tt[STRLEN("-----END ")+n], "-----\n", STRLEN("-----\n")) == 0)
			break;
		tt = strchr(tt, '\n');
		if(tt == nil)
			break;
	}
	if(tt == nil || tt == e){
		werrstr("incorrect .pem file format: bad header or trailer");
		return nil;
	}

	if(new_s)
		*new_s = tt+1;
	n = ((tt - t) * 6 + 7) / 8;
	d = malloc(n);
	if(d == nil){
		werrstr("out of memory");
		return nil;
	}
	n = dec64(d, n, t, tt - t);
	if(n < 0){
		free(d);
		werrstr("incorrect .pem file format: bad base64 encoded data");
		return nil;
	}
	*len = n;
	return d;
}

PEMChain*
decodepemchain(char *s, char *type)
{
	PEMChain *first = nil, *last = nil, *chp;
	u8 *d;
	char *e;
	int n;

	e = strchr(s, '\0');
	while (s < e) {
		d = decodePEM(s, type, &n, &s);
		if(d == nil)
			break;
		chp = malloc(sizeof(PEMChain));
		chp->next = nil;
		chp->pem = d;
		chp->pemlen = n;
		if (first == nil)
			first = chp;
		else
			last->next = chp;
		last = chp;
	}
	return first;
}
