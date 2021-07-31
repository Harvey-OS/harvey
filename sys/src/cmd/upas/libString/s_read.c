#include <u.h>
#include <libc.h>
#include <bio.h>
#include "String.h"

enum
{
	Minread=	256,
};

/* Append up to 'len' input bytes to the string 'to'.
 *
 * Returns the number of characters read.
 */ 
extern int
s_read(Biobuf *fp, String *to, int len)
{
	int rv;
	int n;

	for(rv = 0; rv < len; rv += n){
		n = to->end - to->ptr;
		if(n < Minread){
			s_simplegrow(to, Minread);
			n = to->end - to->ptr;
		}
		if(n > len - rv)
			n = len - rv;
		n = Bread(fp, to->ptr, n);
		if(n <= 0)
			break;
		to->ptr += n;
	}
	s_terminate(to);
	return rv;
}
