#include <u.h>
#include <ctype.h>

/*
 *  return ndb attribute type of an ip name
 */
char*
ipattr(char *name)
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
