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
#include <bio.h>
#include "authcmdlib.h"

/*
 * compute the key verification checksum
 */
void
checksum(char key[], char csum[]) {
	uint8_t buf[8];

	memset(buf, 0, 8);
	encrypt(key, buf, 8);
	sprint(csum, "C %.2x%.2x%.2x%.2x", buf[0], buf[1], buf[2], buf[3]);
}

/*
 * compute the proper response.  We encrypt the ascii of
 * challenge number, with trailing binary zero fill.
 * This process was derived empirically.
 * this was copied from inet's guard.
 */
char *
netresp(char *key, int32_t chal, char *answer)
{
	uint8_t buf[8];

	memset(buf, 0, 8);
	snprint((char *)buf, sizeof buf, "%lu", chal);
	if(encrypt(key, buf, 8) < 0)
		error("can't encrypt response");
	chal = (buf[0]<<24)+(buf[1]<<16)+(buf[2]<<8)+buf[3];
	sprint(answer, "%.8lux", chal);

	return answer;
}

char *
netdecimal(char *answer)
{
	int i;

	for(i = 0; answer[i]; i++)
		switch(answer[i]){
		case 'a': case 'b': case 'c':
			answer[i] = '2';
			break;
		case 'd': case 'e': case 'f':
			answer[i] = '3';
			break;
		}
	return answer;
}

int
netcheck(void *key, int32_t chal, char *response)
{
	char answer[32], *p;
	int i;

	if(smartcheck(key, chal, response))
		return 1;

	if(p = strchr(response, '\n'))
		*p = '\0';
	netresp(key, chal, answer);

	/*
	 * check for hex response -- securenet mode 1 or 5
	 */
	for(i = 0; response[i]; i++)
		if(response[i] >= 'A' && response[i] <= 'Z')
			response[i] -= 'A' - 'a';
	if(strcmp(answer, response) == 0)
		return 1;

	/*
	 * check for decimal response -- securenet mode 0 or 4
	 */
	return strcmp(netdecimal(answer), response) == 0;
}

int
smartcheck(void *key, int32_t chal, char *response)
{
	uint8_t buf[2*8];
	int i, c, cslo, cshi;

	snprint((char*)buf, sizeof buf, "%lu        ", chal);
	cslo = 0x52;
	cshi = cslo;
	for(i = 0; i < 8; i++){
		c = buf[i];
		if(c >= '0' && c <= '9')
			c -= '0';
		cslo += c;
		if(cslo > 0xff)
			cslo -= 0xff;
		cshi += cslo;
		if(cshi > 0xff)
			cshi -= 0xff;
		buf[i] = c | (cshi & 0xf0);
	}

	encrypt(key, buf, 8);
	for(i = 0; i < 8; i++)
		if(response[i] != buf[i] % 10 + '0')
			return 0;
	return 1;
}
