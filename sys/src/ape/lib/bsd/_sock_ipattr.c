/* posix */
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>

/* bsd extensions */
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "priv.h"

/*
 *  return ndb attribute type of an ip name
 */
int
_sock_ipattr(char *name)
{
	char *p;
	int dot = 0;
	int alpha = 0;

	for(p = name; *p; p++){
		if(isdigit(*p))
			;
		else if(isalpha(*p) || *p == '-')
			alpha = 1;
		else if(*p == '.')
			dot = 1;
		else
			return Tsys;
	}

	if(alpha){
		if(dot)
			return Tdom;
		else
			return Tsys;
	}

	if(dot)
		return Tip;
	else
		return Tsys;
}
