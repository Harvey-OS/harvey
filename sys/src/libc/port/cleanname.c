#include <u.h>
#include <libc.h>

/*
 * In place, rewrite name to compress multiple /, eliminate ., and process ..
 */
#define SEP(x)	((x)=='/' || (x) == 0)
char*
cleanname(char *name)
{
	char *s;	/* source of copy */
	char *d;	/* destination of copy */
	char *d0;	/* start of path afer the root name */
	Rune r;
	int rooted;

	if(name[0] == 0)
		return strcpy(name, ".");
	rooted = 0;
	d0 = name;
	if(d0[0] == '#'){
		if(d0[1] == 0)
			return d0;
		d0  += 1 + chartorune(&r, d0+1); /* ignore slash: #/ */
		while(!SEP(*d0))
			d0 += chartorune(&r, d0);
		if(d0 == 0)
			return name;
		d0++;	/* keep / after #<name> */
		rooted = 1;
	}else if(d0[0] == '/'){
		rooted = 1;
		d0++;
	}

	s = d0;
	if(rooted){
		/* skip extra '/' at root name */
		for(; *s == '/'; s++)
			;
	}
	/* remove dup slashes */
	for(d = d0; *s != 0; s++){
		*d++ = *s;
		if(*s == '/')
			while(s[1] == '/')
				s++;
	}
	*d = 0;

	d = d0;
	s = d0;
	while(*s != 0){
		if(s[0] == '.' && SEP(s[1])){
			if(s[1] == 0)
				break;
			s+= 2;
			continue;
		}
		if(s[0] == '.' && s[1] == '.' && SEP(s[2])){
			if(d == d0){
				if(rooted){
					/* /../x -> /x */
					if(s[2] == 0)
						break;
					s += 3;
					continue;
				}else{
					/* ../x -> ../x; and never collect ../ */
					d0 += 3;
				}
			}
			if(d > d0){
				/* a/../x -> x */
				assert(d-2 >= d0 && d[-1] == '/');
				for(d -= 2; d > d0 && d[-1] != '/'; d--)
						;
				if(s[2] == 0)
					break;
				s += 3;
				continue;
			}
		}
		while(!SEP(*s))
			*d++ = *s++;
		if(*s == 0)
			break;
		
		*d++ = *s++;
	}
	*d = 0;
	if(d-1 > name && d[-1] == '/')	/* thanks to #/ */
		*--d = 0;
	if(name[0] == 0)
		strcpy(name, ".");
	return name;
}
