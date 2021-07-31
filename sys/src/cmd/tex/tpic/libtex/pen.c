#include "tex.h"
pen(s) 
char	*s; 
{
	double dlen;

	while (*s != NULL) {
		switch (*s) {
		case 'd':
			if (!(strncmp(s, "dash", 4))) 
				e1->pen = DASHPEN;
			else if (!(strncmp(s, "dot", 3)))
				e1->pen = DOTPEN;
			else
				break;
			while (*s && *s!=' ')
				s++;
			if (sscanf(s, "%f, &dlen)==1 && dlen > 0)
				e1->dashlen = dlen;
			break;
		case 's':
			if (!(strncmp(s, "solid", 5))) 
				e1->pen = SOLIDPEN;
			break;
		}
}
