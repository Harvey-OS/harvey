#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <html.h>
#include "dat.h"

int
inputvalue(Focus *focus, char *val)
{
	Formfield *ff;
	Option *o;
	Rune *rval;
	
	if(focus->type != FocusFormfield){
		print("bad focus in input\n");
		return 0;
	}
	ff = focus->formfield;
	rval = nil;
	switch(ff->ftype){
	case Fsubmit:
	case Fimage:
		/*
		 * XXX have to change submit to expect x, y pair
		 * (perhaps as string) and pass name.x= name.y=
		 * values.
		 */
		print("use submit for submitting");
		return 0;
	case Ftext:
	case Fpassword:
	case Ftextarea:
		break;
	case Fcheckbox:
		if(strcmp(val, "no") != 0 && strcmp(val, "yes") != 0){
			print("checkbox must be yes or no");
			return 0;
		}
		break;
	case Fselect:
		rval = toStr((uchar*)val, strlen(val), UTF_8);
		for(o=ff->options; o; o=o->next){
			if(runestrcmp(rval, o->value) == 0)
				goto okayselect;
			if(runestrcmp(rval, o->display) == 0){
				free(rval);
				rval = runestrdup(o->value);
				goto okayselect;
			}
		}
		print("input: no option \"%s\"", val);
		return 0;
	okayselect:
		break;
	case Fradio:
		print("radio not implemented\n");
		break;
	case Fhidden:
		print("warning: changing hidden field\n");
		break;
	case Freset:
		print("reset not implemented\n");
		break;
	case Ffile:
		print("file not implemented\n");
		break;
	case Fbutton:
		print("button not implemented\n");
		break;
	}
	if(rval == nil)
		rval = toStr((uchar*)val, strlen(val), UTF_8);
	ff->value = rval;
	return 1;
}

int
submitform(Form *form, Formfield *fsubmit)
{
	char *post, *sep, *url;
	int x, y;
	Fmt fmt;
	Formfield *ff;

	fmtstrinit(&fmt);
	if(form->method == HGet){
		fmtprint(&fmt, "%S", form->action);
		sep = "?";
	}else
		sep = "";
	for(ff=form->fields; ff; ff=ff->next){
		if(ff->value == nil || ff->ftype == Freset)
			continue;
		if(ff->name == nil){
			fprint(2, "warning: field %F has no name\n", ff->ftype);
			continue;
		}
		if(ff->ftype == Fsubmit || ff->ftype == Fimage){
			if(fsubmit == nil && ff->ftype == Fsubmit)
				fsubmit = ff;
			else if(fsubmit != ff)
				continue;
		}
		if(ff->ftype == Fimage){
			/* XXX initialize x and y from ff->value*/
			x = y = 0;
			fmtprint(&fmt, "%s%Z.x=%d&%Z.y=%d", 
				sep, ff->name, x, ff->name, y);
		}else
			fmtprint(&fmt, "%s%Z=%Z", sep, ff->name, ff->value);
		sep = "&";	/* prefer ";" but hget doesn't know about it */
	}
	if(form->method == HGet){
		url = fmtstrflush(&fmt);
		post = nil;
	}else{
		url = smprint("%S", form->action);
		post = fmtstrflush(&fmt);
	}
	loadurl(url, post);
	free(url);
	free(post);
	return 1;
}
