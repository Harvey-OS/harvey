/* Copyright 1990, AT&T Bell Labs */

/* Canonicalize the number string pointed to by dp, of length
   len.  Put the result in kp.

   A field of length zero, or all blank, is regarded as 0.
   Over/underflow is rendered as huge or zero and properly signed.
   It happens 1e+-1022.

   Canonicalized strings may be compared as strings of unsigned
   chars.  For good measure, a canonical string has no zero bytes.

   Syntax: optionally signed floating point, with optional
   leading spaces.  A syntax deviation ends the number.

   Form of output: packed in 4-bit nibbles.  First
   3 nibbles count the number N of significant digits
   before the decimal point.  The quantity actually stored
   is 2048+sign(x)*(N+1024).  Further nibbles contain
   1 decimal digit d each, stored as d+2 if x is positive
   and as 10-d if x is negative.  Leading and trailing
   zeros are stripped, and a trailing "digit" d = -1 
   is appended.  (The trailing digit handled like all others,
   so encodes as 1 or 0xb according to the sign of x.)
   An odd number of nibbles is padded with zero.

   Buglet: overflow is reported if output is exactly filled.

*/
#include "fsort.h"

#define encode(x) (neg? 10-(x): (x)+2)
#define putdig(x) (nib? (*dig=encode(x)<<4, nib=0): \
		  (*dig++|=encode(x), nib=1))

int
ncode(uchar *dp, uchar *kp, int len, Field *f)
{
	uchar *dig = kp + 1;	/* byte for next digit */
	int nib = 0;		/* high nibble 1, low nibble 0 */
	uchar *cp = dp;
	uchar *ep = cp + len;	/* end pointer */
	int zeros = 0;		/* count zeros seen but not installed */
	int sigdig = 1024;
	int neg = f->rflag;	/* 0 for +, 1 for - */
	int decimal = 0;
	int n, inv;

	kp[1] = 0;
	for(; cp<ep ; cp++)	/* eat blanks */
		if(*cp!=' ' && *cp!='\t')
			break;
	if(cp < ep)		/* eat sign */
		switch(*cp) {
		case '-':
			neg ^= 1;
		case '+':
			cp++;
		}
	for(; cp<ep; cp++)	/* eat leading zeros */
		if(*cp != '0')
			break;
	if(*cp=='.' && cp<ep) {	/* decimal point among lead zeros */
		decimal++;
		for(cp++; cp<ep; cp++) {
			if(*cp != '0')
				break;
			sigdig--;
		}
	}
	if(*cp>'9' || *cp<'0' || cp>=ep) {	/* no sig digit*/
		sigdig = 0;
		neg = 0;
		goto retzero;
	}
	for(; cp<ep; cp++) {
		switch(*cp) {
		default:
			goto out;
		case '.':
			if(decimal)
				goto out;
			decimal++;
			continue;
		case '0':
			zeros++;
			if(!decimal)
				sigdig++;
			continue;
		case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9':
			for( ; zeros>0; zeros--)
				putdig(0);
			n = *cp - '0';
			putdig(n);
			if(!decimal)
				sigdig++;
			continue;
		case 'e':
		case 'E':
			inv = 1;
			if(cp < ep) switch(*++cp) {
			case '-':
				inv = -1;
			case '+':
				cp++;
			}
			if(*cp>'9' || *cp<'0' || cp>=ep)
				goto out;
			for(n=0; cp<ep; cp++) {
				int c = *cp;

				if(c<'0' || c>'9')
					break;
				if((n = 10*n+c-'0') >= 0)
					continue;
				sigdig = 2047*inv;
				goto out;
			}
			sigdig += n*inv;
			goto out;
		}
	}

out:
	if(sigdig<0 || sigdig>=2047) {
		sigdig = sigdig<0? 0: 2047;
		warn("numeric field overflow", (char*)dp, len);
		dig = kp + 1;
		*dig = 0;
		nib = 0;
	}

retzero:
	if(neg)
		sigdig = 2048 - sigdig;
	else
		sigdig = 2048 + sigdig;
	kp[0] = sigdig >> 4;
	kp[1] |= sigdig << 4;
	putdig(-1);
	return dig - kp + 1 - nib;
}
