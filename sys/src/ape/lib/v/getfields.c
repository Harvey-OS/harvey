#define _RESEARCH_SOURCE
#include <libv.h>
#include <string.h>

static char is_sep[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
static char is_field[256] = {
	0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};
static char last_sep[256];

char *
setfields(char *arg)
{
	register unsigned char *s;
	register i;

	for(i = 1, s = (unsigned char *)last_sep; i < 256; i++)
		if(is_sep[i])
			*s++ = i;
	*s = 0;
	memset(is_sep, 0, sizeof is_sep);
	memset(is_field, 1, sizeof is_field);
	for(s = (unsigned char *)arg; *s;){
		is_sep[*s] = 1;
		is_field[*s++] = 0;
	}
	is_field[0] = 0;
	return(last_sep);
}

int
getfields(char *ss, char **sp, int nptrs)
{
	register unsigned char *s = (unsigned char *)ss;
	register unsigned char **p = (unsigned char **)sp;
	register unsigned c;

	for(;;){
		if(--nptrs < 0) break;
		*p++ = s;
		while(is_field[c = *s++])
			;
		if(c == 0) break;
		s[-1] = 0;
	}
	if(nptrs > 0)
		*p = 0;
	else if(--s >= (unsigned char *)ss)
		*s = c;
	return(p - (unsigned char **)sp);
}

int
getmfields(char *ss, char **sp, int nptrs)
{
	register unsigned char *s = (unsigned char *)ss;
	register unsigned char **p = (unsigned char **)sp;
	register unsigned c;

	if(nptrs <= 0)
		return(0);
	goto flushdelim;
	for(;;){
		*p++ = s;
		if(--nptrs == 0) break;
		while(is_field[c = *s++])
			;
		/*
		 *  s is now pointing 1 past the delimiter of the last field
		 *  c is the delimiter
		 */
		if(c == 0) break;
		s[-1] = 0;
	flushdelim:
		while(is_sep[c = *s++])
			;
		/*
		 *  s is now pointing 1 past the beginning of the next field
		 *  c is the first letter of the field
		 */
		if(c == 0) break;
		s--;
		/*
		 *  s is now pointing to the beginning of the next field
		 *  c is the first letter of the field
		 */
	}
	if(nptrs > 0)
		*p = 0;
	return(p - (unsigned char **)sp);
}

#ifdef	MAIN
#include	<fio.h>

main()
{
	char *fields[256];
	char *s;
	int n, i;
	char buf[1024];

	print("go:\n");
	while(s = Frdline(0)){
		strcpy(buf, s);
		Fprint(1, "getf:");
		n = getfields(s, fields, 4);
		for(i = 0; i < n; i++)
			Fprint(1, " >%s<", fields[i]);
		Fputc(1, '\n');
		Fprint(1, "getmf:");
		n = getmfields(buf, fields, 4);
		for(i = 0; i < n; i++)
			Fprint(1, " >%s<", fields[i]);
		Fputc(1, '\n');
		Fflush(1);
	}
	exit(0);
}
#endif
