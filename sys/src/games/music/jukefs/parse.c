#include <u.h>
#include <libc.h>
#include <thread.h>
#include <bio.h>
#include <ctype.h>
#include "object.h"
#include "catset.h"
#include "parse.h"

#define MAXTOKEN 1024

Biobuf *f;
static int str;
char *file;

Token tokenlistinit[] = {
	{ "category",	Obj,	Category	, "music"	, {nil,0}},
	{ "cddata",	Obj,	Cddata		, nil		, {nil,0}},
	{ "command",	Obj,	Cmd		, nil		, {nil,0}},
	{ "file",	Obj,	File		, "file"	, {nil,0}},
	{ "include",	Obj,	Include		, nil		, {nil,0}},
	{ "key",	Obj,	Key		, nil		, {nil,0}},
	{ "lyrics",	Obj,	Lyrics		, "lyrics"	, {nil,0}},
	{ "part",	Obj,	Part		, "title"	, {nil,0}},
	{ "path",	Obj,	Path		, nil		, {nil,0}},
	{ "performance",Obj,	Performance	, "artist"	, {nil,0}},
	{ "recording",	Obj,	Recording	, "title"	, {nil,0}},
	{ "root",	Obj,	Root		, nil		, {nil,0}},
	{ "search",	Obj,	Search		, nil		, {nil,0}},
	{ "soloists",	Obj,	Soloists	, "artist"	, {nil,0}},
	{ "time",	Obj,	Time		, "time"	, {nil,0}},
	{ "track",	Obj,	Track		, "title"	, {nil,0}},
	{ "work",	Obj,	Work		, "title"	, {nil,0}},
};
Token *tokenlist;
int ntoken = nelem(tokenlistinit);
int catnr = 0;

Cmdlist cmdlist[] = {
	{	Sort,	"sort"		},
	{	Enum,	"number"	},
	{	0x00,	0		},
};

static char *curtext;

void
inittokenlist(void)
{
	int i;

	ntoken = nelem(tokenlistinit);
	tokenlist = malloc(sizeof(tokenlistinit));
	memmove(tokenlist, tokenlistinit, sizeof(tokenlistinit));
	for(i = 0; i< ntoken; i++){
		tokenlist[i].name = strdup(tokenlist[i].name);
		catsetinit(&tokenlist[i].categories, tokenlist[i].value);
	}
	curtext = smprint("{");
}

Type
gettoken(char *token)
{
	char *p, *q;
	int i, n;
	Token *t;

	for(;;){
		if(curtext){
			p = &curtext[strspn(curtext, " \t")];	
			if(*p && *p != '\n')
				break;
		}
		do {
			str++;
			free(curtext);
			if((curtext = Brdstr(f, '\n', 0)) == nil)
				return Eof;
		} while(curtext[0] == '#');
	}
	if(*p == '{'){
		*token++ = *p;
		*token = 0;
		*p = ' ';
		return BraceO;
	}
	if(*p == '}'){
		*token++ = *p;
		*token = 0;
		*p = ' ';
		return BraceC;
	}
	if(*p == '='){
		*token++ = *p;
		*token = 0;
		*p = ' ';
		return Equals;
	}
	t = nil;
	n = 0;
	for(i = 0; i < ntoken; i++){
		t = &tokenlist[i];
		if(strncmp(p, t->name, n=strlen(t->name)) == 0){
			q = &p[n];
				if(isalnum(*q) || *q == '-') continue;
			q += strspn(q, " \t");
			if(t->kind == Obj && *q == '{')
				break;
			if(t->kind == Cat && *q == '=')
				break;
		}
	}
	if(i < ntoken){
		strcpy(token, t->name);
		memset(p, ' ', n);
		return i;
	}
	assert(strlen(token) < MAXTOKEN);
	if(strchr(p, '{'))
		sysfatal("Illegal keyword or parse error: %s", p);
	if((q = strchr(p, '='))){
		if(q == p) goto tx;
		*q = 0;
		strcpy(token, p);
		assert(strlen(token) < MAXTOKEN);
		memset(p, ' ', q-p);
		*q = '=';
		for(q = token; *q; q++)
			if(!isalnum(*q) && !isspace(*q)) break;
		if(*q) return Txt;
		while(isspace(*--q)) *q = 0;
		return Newcat;
	}
tx:	if((q = strchr(p, '}'))){
		*q = 0;
		strcpy(token, p);
		assert(strlen(token) < MAXTOKEN);
		memset(p, ' ', q-p);
		*q = '}';
		return Txt;
	}
	strcpy(token, p);
	assert(strlen(token) < MAXTOKEN);
	free(curtext);
	curtext = nil;
	return Txt;
}

Object *
getobject(Type t, Object *parent)
{
	char *token;
	char *textbuf;
	char *tp, *p, *q;
	int i;
	Object *o, *oo, *child;
	Token *ot;
	Type nt;

	token = malloc(MAXTOKEN);
	textbuf = malloc(8192);

	tp = textbuf;
	o = newobject(t, parent);
	o->flags |= Hier;
	if(parent == nil){
		root = o;
		o->path = strdup(startdir);
		setmalloctag(o->path, 0x100001);
	}
	if(gettoken(token) != BraceO)
		sysfatal("Parse error: no brace, str %d", str);
	for(;;){
		t = gettoken(token);
		if(t >= 0)
			switch(tokenlist[t].kind){
			case Obj:
				switch(t){
				case Key:
				case Cmd:
				case Path:
					if(getobject(t, o) != nil)
						sysfatal("Non-null child?");
					break;
				case Include:
				case Category:
					child = getobject(t, o);
					if(child) addchild(o, child, "case Category");
					break;
				default:
					/* subobject */
					child = getobject(t, o);
					if(child == nil)
						sysfatal("Null child?");
					addchild(o, child, "default");
					break;
				}
				break;
			case Cat:
			catcase:    nt = gettoken(token);
				if(nt != Equals)
					sysfatal("Expected Equals, not %s", token);
				nt = gettoken(token);
				if(nt != Txt)
					sysfatal("Expected Text, not %s", token);
				if((p = strchr(token, '\n'))) *p = 0;
				p = token;
				if(o->type == Category){
					if(catsetisset(&o->categories)){
						fprint(2, "Category object must have one category\n");
					}
					catsetcopy(&o->categories, &tokenlist[t].categories);
					strncpy(o->key, p, KEYLEN);
					if(catobjects[t] == 0)
						sysfatal("Class %s not yet defined", tokenlist[t].name);
					for(i = 0; i < catobjects[t]->nchildren; i++)
						if(strcmp(catobjects[t]->children[i]->key, p) == 0)
							break;
					if(i == catobjects[t]->nchildren){
						/* It's a new key for the category */
						addchild(catobjects[t], o, "new key for cat");
					}else{
						/* Key already existed */
						oo = catobjects[t]->children[i];
						if(oo->value)
							sysfatal("Duplicate category object for %s", oo->value);
						catobjects[t]->children[i] = o;
						if(oo->nchildren){
							for(i = 0; i < oo->nchildren; i++){
								if(oo->children[i]->parent == oo)
									oo->children[i]->parent = o;
								addchild(o, oo->children[i], "key already existed");
							}
						}
						freeobject(oo, "a");
					}
					o->parent = catobjects[t];
				}else{
					catsetorset(&o->categories, &tokenlist[t].categories);
					for(i = 0; i < catobjects[t]->nchildren; i++)
						if(strcmp(catobjects[t]->children[i]->key, p) == 0)
							break;
					if(i == catobjects[t]->nchildren){
						oo = newobject(Category, catobjects[t]);
/*
						oo->value = strdup(token);
*/
						strncpy(oo->key, p, KEYLEN);
						catsetcopy(&oo->categories, &tokenlist[t].categories);
						addchild(catobjects[t], oo, "catobjects[t],oo");
					}
					addchild(catobjects[t]->children[i], o, "children[i]");
				}
				break;
			}
		else
			switch(t){
			case Eof:
				if(o->type == Root){
					free(token);
					free(textbuf);
					return o;
				}
				sysfatal("Unexpected Eof in %s, file %s", tokenlist[o->type].name, file);
			case Newcat:
				/* New category, make an entry in the tokenlist */
				tokenlist = realloc(tokenlist, (ntoken+1)*sizeof(Token));
				ot = &tokenlist[ntoken];
				ot->name = strdup(token);
				setmalloctag(ot->name, 0x100002);
				ot->kind = Cat;
				ot->value = -1;
				memset(&ot->categories, 0, sizeof(Catset));
				catsetinit(&ot->categories, catnr++);
				/* And make an entry in the catobjects table */
				if(ncat <= ntoken){
					catobjects = realloc(catobjects, (ntoken+1)*sizeof(Object*));
					while(ncat <= ntoken) catobjects[ncat++] = nil;
				}
				if(catobjects[ntoken] != nil)
					sysfatal("Class %s already defined in %s:%d", token, file, str);
				if(0) fprint(2, "newcat: token %s catnr %d ntoken %d ncat %d\n",
					token, catnr, ntoken, ncat);
				catobjects[ntoken] = newobject(Category, root);
				if(o->type == Category)
					catobjects[ntoken]->flags = o->flags&Hier;
				catobjects[ntoken]->flags |= Sort;
				strncpy(catobjects[ntoken]->key, token, KEYLEN);
				catsetcopy(&catobjects[ntoken]->categories, &ot->categories);
				addchild(root, catobjects[ntoken], "root");
				t = ntoken;
				ntoken++;
				goto catcase;
			case Txt:
				strcpy(tp, token);
				tp += strlen(token);
				break;
			case BraceC:
				while(tp > textbuf && tp[-1] == '\n') *--tp = 0;
				if((o->type == File || o->type == Include) && o->path){
					o->value = smprint("%s/%s", o->path, textbuf);
				}else if(tp > textbuf){
					o->value = strdup(textbuf);
					setmalloctag(o->value, 0x100003);
				}
				switch(o->type){
				case Cmd:
					q = strtok(o->value, " \t,;\n");
					while(q){
						if(*q) for(i = 0; cmdlist[i].name; i++){
							if(strcmp(q, cmdlist[i].name) == 0){
								o->parent->flags |= cmdlist[i].flag;
								break;
							}
							if(cmdlist[i].name == 0)
								fprint(2, "Unknown command: %s\n", q);
						}
						q = strtok(nil, " \t,;\n");
					}
					freeobject(o, "b");
					free(token);
					free(textbuf);
					return nil;
				case Path:
					p = o->value;
					free(o->parent->path);
					if(p[0] == '/' || o->path == nil){
						o->parent->path = strdup(p);
						setmalloctag(o->parent->path, 0x100004);
					}else{
						o->parent->path = smprint("%s/%s", o->path, p);
						setmalloctag(o->parent->path, 0x100005);
					}
					freeobject(o, "b");
					free(token);
					free(textbuf);
					return nil;
				case Include:
					free(token);
					free(textbuf);
					return getinclude(o);
				case Category:
				/*
					if(o->nchildren) break;
				 */
					free(token);
					free(textbuf);
					return nil;
				case Key:
					strncpy(o->parent->key, o->value, KEYLEN);
					freeobject(o, "d");
					free(token);
					free(textbuf);
					return nil;
				default:
					break;
				}
				free(token);
				free(textbuf);
				return o;
			default:
				fprint(2, "Unexpected token: %s\n", token);
				free(token);
				free(textbuf);
				return nil;
			}
	}
}

Object *
getinclude(Object *o)
{
		char *savetext;
		Biobuf *savef = f;
		char *savefile, fname[256];
		Object *oo;
		int savestr = str;
		char token[MAXTOKEN], *dirname, *filename;
		Type t;

		str = 0;
		if(curtext){
			savetext = strdup(curtext);
			setmalloctag(savetext, 0x100006);
		}else
			savetext = nil;
		if((f = Bopen(o->value, OREAD)) == nil)
			sysfatal("getinclude: %s: %r", o->value);
		savefile = file;
		file = strdup(o->value);
		strncpy(fname, o->value, 256);
		if((filename = strrchr(fname, '/'))){
			*filename = 0;
			dirname = fname;
			filename++;
		}else{
			dirname = "";
			filename = fname;
		}
		while((t = gettoken(token)) != Eof){
			if(t < 0){
				if(*dirname)
					sysfatal("Bad include file %s/%s, token %s, str %d",
						dirname, filename, token, str);
				else
					sysfatal("Bad include file %s, token %s, str %d",
						filename, token, str);
			}
			free(o->path);
			o->path = strdup(dirname);
			setmalloctag(o->path, 0x100007);
			oo = getobject(t, o->parent);
			if(oo) addchild(o->parent, oo, "o->parent, oo");
		}
		freeobject(o, "e");
		free(curtext);
		curtext = nil;
		if(savetext)
			curtext = savetext;
		free(file);
		file = savefile;
		str = savestr;
		Bterm(f);
		f = savef;
		return nil;
}

void
addchild(Object *parent, Object *child, char *where)
{
		int i;

		/* First check if child's already been added
		 * This saves checking elsewhere
		 */
		for(i = 0; i < parent->nchildren; i++)
				if(parent->children[i] == child) return;
		parent->children = realloc(parent->children, (i+1)*4);
		parent->children[i] = child;
		parent->nchildren++;
		if(parent->type == Category && child->type == Category)
			return;
		if(parent->type == Work && child->type == Work)
			return;
		if(parent->type == Work && child->type == Track)
			return;
		if(parent->type == Track && child->type == File)
			return;
		if(child->parent == child)
			return;
		if(parent->type == Root)
			return;
		if(parent->parent->type == Root)
			return;
//		addcatparent(parent, child);
		i = child->ncatparents;
		if(0) fprint(2, "addcatparent %s parent %d type %d child %d type %d\n",where,
			parent->tabno, parent->type, child->tabno, child->type);
		child->catparents = realloc(child->catparents, (i+1)*4);
		child->catparents[i] = parent;
		child->ncatparents++;
}

void
addcatparent(Object *parent, Object *child)
{
		int i;

		/* First check if child's already been added
		 * This saves checking elsewhere
		 */
		if(child->parent == child)
			return;
//		for(i = 0; i < child->ncatparents; i++)
//				if(child->catparents[i] == parent) return;
		i = child->ncatparents;
		fprint(2, "addcatparent parent %d child %d\n", parent->tabno, child->tabno);
		child->catparents = realloc(child->catparents, (i+1)*4);
		child->catparents[i] = parent;
		child->ncatparents++;
}

void
sortprep(char *out, int n, Object *o)
{
	char *p, *q;

	if(*o->key)
		q = o->key;
	else if (o->value)
		q = o->value;
	else
		q = "";
	if(p = strchr(q, '~'))
		p++;
	else
		p = q;
	for(q = out; *p && q < out+n-1; q++)
		*q = tolower(*p++);
	*q = 0;
}

void
childsort(Object *o)
{
		Object *oo;
		int i, j, n;
		char si[256], sj[256];
		/* sort the kids by key or by value */

		n = o->nchildren;
		if(n > 1){
			for(i = 0; i < n-1; i++){
				sortprep(si, nelem(si), o->children[i]);
				for(j = i+1; j < n; j++){
					sortprep(sj, nelem(sj), o->children[j]);
					if(strncmp(si, sj, sizeof(si)) > 0){
						oo = o->children[i];
						o->children[i] = o->children[j];
						o->children[j] = oo;
						strncpy(si, sj, sizeof(si));
					}
				}
			}
		}
}

void
childenum(Object *o){
		Object *oo;
		int i, n = 1;

		for(i = 0; i < o->nchildren; i++){
			oo = o->children[i];
			if(tokenlist[oo->type].kind == Cat)
				oo->num = n++;
			else
				switch(oo->type){
				case Category:
				case Part:
				case Recording:
				case Track:
				case Work:
					oo->num = n++;
				default:
					break;
				}
		}
}

Object *
newobject(Type t, Object *parent){
	Object *o;
	int tabno;

	if(hotab){
		for(tabno = 0; tabno < notab; tabno++)
			if(otab[tabno] == nil)
				break;
		if(tabno == notab)
			sysfatal("lost my hole");
		hotab--;
	}else{
		if(sotab < notab+1){
			sotab += 512;
			otab = realloc(otab, sizeof(Object*)*sotab);
			if(otab == nil)
				sysfatal("realloc: %r");
		}
		tabno = notab++;
	}
	o = mallocz(sizeof(Object), 1);
	o->tabno = tabno;
	otab[tabno] = o;
	o->type = t;
	o->parent = parent;
	if(parent && parent->path){
		o->path = strdup(parent->path);
		setmalloctag(o->path, 0x100008);
	}
	return o;
}

void
freeobject(Object *o, char*){

	free(o->children);
	if(o->orig == nil)
		free(o->value);
	free(o->path);
	free(o->catparents);
	catsetfree(&o->categories);
	otab[o->tabno] = nil;
	hotab++;
	free(o);
}

void
freetree(Object *o)
{
	int i;

	for(i = 0; i < o->nchildren; i++)
		if(o->children[i]->parent == o)
			freetree(o->children[i]);
	free(o->children);
	if(o->orig == nil)
		free(o->value);
	free(o->path);
	free(o->catparents);
	catsetfree(&o->categories);
	otab[o->tabno] = nil;
	hotab++;
	free(o);
}
