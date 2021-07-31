#include <alef.h>

/*
 *  return ndb attribute type of an ip name
 */
byte*
ipattr(byte *name)
{
	byte *p;
	int dot, alpha;

	dot = 0;
	alpha = 0;
	for(p = name; *p; p++){
		if(*p >= '0' && *p <= '9')
			;
		else if((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '-')
			alpha = 1;
		else if(*p == '.')
			dot = 1;
		else
			return "sys";
	}

	if(alpha){
		if(dot)
			return "dom";
		else
			return "sys";
	}

	if(dot)
		return "ip";
	else
		return "sys";
}
