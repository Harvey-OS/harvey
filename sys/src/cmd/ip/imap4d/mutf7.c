#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include "imap4d.h"

/*
 * modified utf-7, as per imap4 spec
 * like utf-7, but substitues , for / in base 64,
 * does not allow escaped ascii characters.
 *
 * /lib/rfc/rfc2152 is utf-7
 * /lib/rfc/rfc1642 is obsolete utf-7
 *
 * test sequences from rfc1642
 *	'A≢Α.'		'A&ImIDkQ-.'
 *	'Hi Mom ☺!"	'Hi Mom &Jjo-!'
 *	'日本語'		'&ZeVnLIqe-'
 */

static uchar mt64d[256];
static char mt64e[64];

static void
initm64(void)
{
	int c, i;

	memset(mt64d, 255, 256);
	memset(mt64e, '=', 64);
	i = 0;
	for(c = 'A'; c <= 'Z'; c++){
		mt64e[i] = c;
		mt64d[c] = i++;
	}
	for(c = 'a'; c <= 'z'; c++){
		mt64e[i] = c;
		mt64d[c] = i++;
	}
	for(c = '0'; c <= '9'; c++){
		mt64e[i] = c;
		mt64d[c] = i++;
	}
	mt64e[i] = '+';
	mt64d['+'] = i++;
	mt64e[i] = ',';
	mt64d[','] = i;
}

int
encmutf7(char *out, int lim, char *in)
{
	Rune rr;
	ulong r, b;
	char *start = out;
	char *e = out + lim;
	int nb;

	if(mt64e[0] == 0)
		initm64();
	for(;;){
		r = *(uchar*)in;

		if(r < ' ' || r >= Runeself){
			if(r == '\0')
				break;
			if(out + 1 >= e)
				return -1;
			*out++ = '&';
			b = 0;
			nb = 0;
			for(;;){
				in += chartorune(&rr, in);
				r = rr;
				if(r == '\0' || r >= ' ' && r < Runeself)
					break;
				b = (b << 16) | r;
				for(nb += 16; nb >= 6; nb -= 6){
					if(out + 1 >= e)
						return -1;
					*out++ = mt64e[(b>>(nb-6))&0x3f];
				}
			}
			for(; nb >= 6; nb -= 6){
				if(out + 1 >= e)
					return -1;
				*out++ = mt64e[(b>>(nb-6))&0x3f];
			}
			if(nb){
				if(out + 1 >= e)
					return -1;
				*out++ = mt64e[(b<<(6-nb))&0x3f];
			}

			if(out + 1 >= e)
				return -1;
			*out++ = '-';
			if(r == '\0')
				break;
		}else
			in++;
		if(out + 1 >= e)
			return -1;
		*out = r;
		out++;
		if(r == '&')
			*out++ = '-';
	}

	if(out >= e)
		return -1;
	*out = '\0';
	return out - start;
}

int
decmutf7(char *out, int lim, char *in)
{
	Rune rr;
	char *start = out;
	char *e = out + lim;
	int c, b, nb;

	if(mt64e[0] == 0)
		initm64();
	for(;;){
		c = *in;

		if(c < ' ' || c >= Runeself){
			if(c == '\0')
				break;
			return -1;
		}
		if(c != '&'){
			if(out + 1 >= e)
				return -1;
			*out++ = c;
			in++;
			continue;
		}
		in++;
		if(*in == '-'){
			if(out + 1 >= e)
				return -1;
			*out++ = '&';
			in++;
			continue;
		}

		b = 0;
		nb = 0;
		while((c = *in++) != '-'){
			c = mt64d[c];
			if(c >= 64)
				return -1;
			b = (b << 6) | c;
			nb += 6;
			if(nb >= 16){
				rr = b >> (nb - 16);
				nb -= 16;
				if(out + UTFmax + 1 >= e && out + runelen(rr) + 1 >= e)
					return -1;
				out += runetochar(out, &rr);
			}
		}
		if(b & ((1 << nb) - 1))
			return -1;
	}

	if(out >= e)
		return -1;
	*out = '\0';
	return out - start;
}
