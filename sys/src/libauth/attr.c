#include <u.h>
#include <libc.h>
#include <String.h>
#include <auth.h>

int
_attrfmt(Fmt *fmt)
{
	char *b, buf[1024], *ebuf;
	Attr *a;

	ebuf = buf+sizeof buf;
	b = buf;
	strcpy(buf, " ");
	for(a=va_arg(fmt->args, Attr*); a; a=a->next){
		if(a->name == nil)
			continue;
		switch(a->type){
		case AttrQuery:
			b = seprint(b, ebuf, " %q?", s_to_c(a->name));
			break;
		case AttrNameval:
			b = seprint(b, ebuf, " %q=%q", s_to_c(a->name), s_to_c(a->val));
			break;
		case AttrDefault:
			b = seprint(b, ebuf, " %q:=%q", s_to_c(a->name), s_to_c(a->val));
			break;
		}
	}
	return fmtstrcpy(fmt, buf+1);
}

Attr*
_copyattr(Attr *a)
{
	Attr **la, *na;

	na = nil;
	la = &na;
	for(; a; a=a->next){
		*la = _mkattr(a->type, s_to_c(a->name), s_to_c(a->val), nil);
		setmalloctag(*la, getcallerpc(&a));
		la = &(*la)->next;
	}
	*la = nil;
	return na;
}

Attr*
_delattr(Attr *a, char *name)
{
	Attr *fa;
	Attr **la;

	for(la=&a; *la; ){
		if(strcmp(s_to_c((*la)->name), name) == 0){
			fa = *la;
			*la = (*la)->next;
			fa->next = nil;
			_freeattr(fa);
		}else
			la=&(*la)->next;
	}
	return a;
}

Attr*
_findattr(Attr *a, char *n)
{
	for(; a; a=a->next)
		if(strcmp(s_to_c(a->name), n) == 0 && a->type != AttrQuery)
			return a;
	return nil;
}

void
_freeattr(Attr *a)
{
	Attr *anext;

	for(; a; a=anext){
		anext = a->next;
		s_free(a->name);
		s_free(a->val);
		a->name = (void*)~0;
		a->val = (void*)~0;
		a->next = (void*)~0;
		free(a);
	}
}

Attr*
_mkattr(int type, char *name, char *val, Attr *next)
{
	Attr *a;

	a = malloc(sizeof(*a));
	if(a==nil)
		sysfatal("_mkattr malloc: %r");
	a->type = type;
	a->name = s_copy(name);
	a->val = s_copy(val);
	a->next = next;
	setmalloctag(a, getcallerpc(&type));
	return a;
}

static Attr*
cleanattr(Attr *a)
{
	Attr *fa;
	Attr **la;

	for(la=&a; *la; ){
		if((*la)->type==AttrQuery && _findattr(a, s_to_c((*la)->name))){
			fa = *la;
			*la = (*la)->next;
			fa->next = nil;
			_freeattr(fa);
		}else
			la=&(*la)->next;
	}
	return a;
}

Attr*
_parseattr(char *s)
{
	char *p, *t, *tok[256];
	int i, ntok, type;
	Attr *a;

	s = strdup(s);
	if(s == nil)
		sysfatal("_parseattr strdup: %r");

	ntok = tokenize(s, tok, nelem(tok));
	a = nil;
	for(i=ntok-1; i>=0; i--){
		t = tok[i];
		if(p = strchr(t, '=')){
			*p++ = '\0';
		//	if(p-2 >= t && p[-2] == ':'){
		//		p[-2] = '\0';
		//		type = AttrDefault;
		//	}else
				type = AttrNameval;
			a = _mkattr(type, t, p, a);
			setmalloctag(a, getcallerpc(&s));
		}
		else if(t[strlen(t)-1] == '?'){
			t[strlen(t)-1] = '\0';
			a = _mkattr(AttrQuery, t, "", a);
			setmalloctag(a, getcallerpc(&s));
		}else{
			/* really a syntax error, but better to provide some indication */
			a = _mkattr(AttrNameval, t, "", a);
			setmalloctag(a, getcallerpc(&s));
		}
	}
	free(s);
	return cleanattr(a);
}

char*
_str_findattr(Attr *a, char *n)
{
	a = _findattr(a, n);
	if(a == nil)
		return nil;
	return s_to_c(a->val);
}

