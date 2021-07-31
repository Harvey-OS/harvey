#include "tex.h"
text(s)
char	*s;
{
	register char *p;
	int centered, right, newline, more;
	double y;

	while(1){
	centered = right = newline = more = tweek = 0;
	for(p=s; *p != '\0'; p++){
		if(*p == '\\'){
			switch(*(++p)){
			case 'C': centered++;
				s = p+1;
				continue;
			case 'R': right++;
				s = p+1;
				continue;
			case 'n': newline++;
				*(p-1) = '\0';
				if(*(p+1) != '\0')more++;
				goto output;
			case 'L': s=p+1;
				continue;
			}
		}
	}
output:
	*p = '\0';
	fprintf(TEXFILE,"    \\rlap{\\kern %6.3fin\\lower%6.3fin\\hbox to 0pt{",
			INCHES(SCX(e1->copyx)), INCHES(e1->copyy));
	fprintf(TEXFILE,centered?"\\hss%s\\hss":(right?"\\hss%s":"%s\\hss"),s);
	if(newline){
		y = TRY(e1->copyy) ;
		y -= (e1->psize+2) / 72.27;
		e1->copyy = (y - e1->bottom)/e1->scaley + e1->ymin;
	}
	if(!more)break;
	s = p+1;
	}
}
