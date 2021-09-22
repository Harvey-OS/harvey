#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <regexp.h>
#include <html.h>
#include <ctype.h>
#include "dat.h"

static char*
wherenames[] = {
	"",
	"first",
	"inner",
	"outer",
	"prev",
	"next"
};
int
wherefmt(Fmt *fmt)
{
	int i;
	
	i = va_arg(fmt->args, int);
	if(i >= 0 && i < nelem(wherenames))
		return fmtstrcpy(fmt, wherenames[i]);
	return fmtprint(fmt, "where%d", i);
}


static char*
thingnames[] = {
	"",
	"cell",
	"checkbox",
	"form",
	"page",
	"password",
	"radiobutton",
	"row",
	"select",
	"table",
	"text",
	"textarea",
	"textbox"
};
int
thingfmt(Fmt *fmt)
{
	int i;
	
	i = va_arg(fmt->args, int);
	if(i >= 0 && i < nelem(thingnames))
		return fmtstrcpy(fmt, thingnames[i]);
	return fmtprint(fmt, "thing%d", i);
}

static char*
fieldnames[] = {
	"textbox",
	"password",
	"checkbox",
	"radio",
	"submit",
	"hidden",
	"image",
	"reset",
	"file",
	"button",
	"select",
	"textarea"
};
int
formfieldfmt(Fmt *fmt)
{
	int i;
	
	i = va_arg(fmt->args, int);
	if(i >= 0 && i < nelem(fieldnames))
		return fmtstrcpy(fmt, fieldnames[i]);
	return fmtprint(fmt, "field%d", i);
}

static void
urlencode(Fmt *fmt, char *s)
{
	int x;
	
	for(; *s; s++){
		x = (uchar)*s;
		if(x == ' ')
			fmtrune(fmt, '+');
		else if(('a' <= x && x <= 'z') || ('A' <= x && x <= 'Z') || ('0' <= x && x <= '9')
			|| strchr("$-_.+!*'()", x)){
			fmtrune(fmt, x);
		}else
			fmtprint(fmt, "%%%02ux", x);
	}
}

int
urlencodefmt(Fmt *fmt)
{
	char *s;
	
	s = va_arg(fmt->args, char*);
	urlencode(fmt, s);
	return 0;
}

int
runeurlencodefmt(Fmt *fmt)
{
	char *s;
	Rune *r;
	
	r = va_arg(fmt->args, Rune*);
	s = smprint("%S", r);
	urlencode(fmt, s);
	free(s);
	return 0;
}

static char escaper[] = "abnrtv\"";
static char escapee[] = "\a\b\n\r\t\v\"";

int
cencodefmt(Fmt *fmt)
{
	char *s, *p;
	Rune r;
	int n;
	
	s = va_arg(fmt->args, char*);
	for(; *s; s+=n){
		n = chartorune(&r, s);
		if(r < Runeself && (p=strchr(escapee, r)) != nil){
			fmtrune(fmt, '\\');
			fmtrune(fmt, escaper[p-escapee]);
		}else
			fmtrune(fmt, r);
	}
	return 0;
}

