#include "agent.h"

#pragma varargck argpos err 4

enum {
	BLANK, FILE, DATA, PROTO, SYM, FLAG, ERR
};

static int
myparse(char *s, char **arg)
{
	char *p, *f[10];
	int eq, nf, inquote, inspace;

	if(p = strchr(s, '#'))
		*p = '\0';

	nf = 0;
	inspace = 1;
	inquote = 0;
	eq = 0;
	for(p=s; *p && nf<5; p++) {
		switch(*p){
		case '"':
			if(!inquote && inspace) {
				inspace = 0;
				f[nf++] = p;
			}
			inquote = !inquote;
			break;
		case '=':
			if(inquote)
				break;
			if(eq != 0 || nf != 1) {
				werrstr("bad assignment");
				return ERR;
			}
			eq = 1;
			goto Space;
			break;
		case ' ':
		case '\t':
		Space:
			if(!inquote) {
				inspace = 1;
				*p = '\0';
			}
			break;
		default:
			if(inquote)
				break;
			if(inspace) {
				inspace = 0;
				f[nf++] = p;
			}
			break;
		}
	}

	if(eq) {
		/* want non-quoted string then quoted string */
		if(nf != 2 || f[1][0] != '"' || f[0][0] == '"') {
			werrstr("bad assignment");
			return ERR;
		}
		arg[0] = f[0];
		arg[1] = f[1];
		return SYM;
	}

	if(nf == 0)
		return BLANK;

	if(nf != 2){
		werrstr("bad number of fields");
		return ERR;
	}

	if(strcmp(f[0], "file") == 0) {
		if(f[1][0] != '/') {
			werrstr("bad file name");
			return ERR;
		}
		arg[0] = f[1];
		return FILE;
	}

	if(strcmp(f[0], "protocol") == 0) {
		if(f[1][0] == '"' || f[1][0] == '$') {
			werrstr("bad protocol name");
			return ERR;
		}
		arg[0] = f[1];
		return PROTO;
	}

	if(strcmp(f[0], "flag") == 0){
		arg[0] = f[1];
		return FLAG;
	}

	if(strcmp(f[0], "data") == 0) {
		if(f[1][0] != '"' && f[1][0] != '$') {
			werrstr("data must be quoted or a variable");
			return ERR;
		}
		arg[0] = f[1];
		return DATA;
	}

	werrstr("unrecognized keyword");
	return ERR;			
}

static Symbol*
lookup(Config *c, char *name)
{
	int i;

	for(i=0; i<c->nsym; i++)
		if(strcmp(c->sym[i].name, name) == 0)
			return &c->sym[i];
	return nil;
}

static Key*
xlookupkey(Config *c, char *file)
{
	int i;

	for(i=0; i<c->nkey; i++)
		if(strcmp(c->key[i]->file, file) == 0)
			return c->key[i];
	return nil;
}

static Symbol*
addsymbol(Config *c, char *name, String *data)
{
	char tmp[30];
	static int symgen;

	if(name == nil) {
		sprint(tmp, "xyzzy%d", ++symgen);
		name = tmp;
	}

	if(lookup(c, name)) {
		werrstr("redefinition of variable %s", name);
		return nil;
	}

	if((c->nsym%8) == 0)
		c->sym = erealloc(c->sym, sizeof(c->sym[0])*(c->nsym+8));

	c->sym[c->nsym].name = estrdup(name);
	c->sym[c->nsym].val = data;
	return &c->sym[c->nsym++];
}

static Proto*
findproto(char *name)
{
	Proto **p;

	for(p=prototab; *p; p++)
		if(strcmp((*p)->name, name) == 0)
			return *p;
	return nil;
}

static int
addkey(Config *c, char *file, char *proto, Symbol *sym, int flag)
{
	Key *k;
	Proto *p;

	if(file == nil && proto == nil && sym == nil)
		return 0;

	if(file == nil || proto == nil || sym == nil) {
		werrstr("incomplete block");
		return -1;
	}

	if(xlookupkey(c, file)) {
		werrstr("duplicate key %s", file);
		return -1;
	}

	if(proto == nil) {
		werrstr("no protocol for key %s", file);
		return -1;
	}

	if((p = findproto(proto)) == nil) {
		werrstr("unknown protocol %s for key %s", proto, file);
		return -1;
	}

	if(sym == nil) {
		werrstr("no data for key %s", file);
		return -1;	
	}

	k = emalloc(sizeof(*k));
	k->file = estrdup(file);
	k->proto = p;
	k->data = sym->val;
	k->sym = sym;
	k->flags = flag;

	if((c->nkey%8) == 0)
		c->key = erealloc(c->key, sizeof(c->key[0])*(c->nkey+8));
	c->key[c->nkey++] = k;

	return 0;
}

static Symbol*
getsym(Config *cstrings, Config *c, char *name, char *s)
{
	String *str;
	Symbol *sym;

	switch(s[0]) {
	case '"':
		if(s[strlen(s)-1] != '"')
			assert(0);
		s[strlen(s)-1] = '\0';
		str = s_copy(s+1);
		if((sym=addsymbol(c, name, str)) == nil) {
			s_free(str);
			return nil;
		}
		return sym;

	case '$':
		if((sym = lookup(c, s+1)) == nil) {
			if((sym = lookup(cstrings, s+1)) == nil) {
				werrstr("unknown variable %s", s);
				return nil;
			}
			if((sym=addsymbol(c, s+1, sym->val)) == nil) {
				werrstr("couldn't copy string %s (can't happen)", s+1);
				return nil;
			}
		}
		return sym;

	default:
		werrstr("bad data format");
		return nil;
	}
}

struct {
	char *name;
	int val;
} flag[] = {
	"confirmopen", Fconfirmopen,
	"confirmuse", Fconfirmuse,
};

static int
getflag(char *s)
{
	int i;

	for(i=0; i<nelem(flag); i++)
		if(strcmp(s, flag[i].name)==0)
			return flag[i].val;
	return -1;
}

Config*
strtoconfig(Config *cstrings, String *s)
{
	Config *c;
	char *p, *nextp, *arg[2];
	int flag, line, r;
	char *file, *proto;
	Symbol *sym;

	c = emalloc(sizeof(*c));
	s = s_clone(s);

	line = 1;
	file = nil;
	flag = 0;
	proto = nil;
	sym = nil;
	for(p=s_to_c(s); p; p=nextp, line++) {
		if(nextp= strchr(p, '\n'))
			*nextp++ = '\0';

		switch(myparse(p, arg)) {
		case ERR:
			threadprint(2, "myparse error: %r\n");
		Error:
			threadprint(2, "agent parse error: %r\n");
			s_free(s);
			freeconfig(c);
			return nil;

		case BLANK:
			// new rule set
			if(addkey(c, file, proto, sym, flag) < 0)
				goto Error;
			file = nil;
			proto = nil;
			sym = nil;
			flag = 0;
			break;

		case FILE:
			file = arg[0];
			break;

		case DATA:
			if((sym = getsym(cstrings, c, nil, arg[0])) == nil)
				goto Error;
			break;

		case PROTO:
			proto = arg[0];
			break;

		case FLAG:
			if((r = getflag(arg[0])) < 0)
				goto Error;
			flag |= r;
			break;

		case SYM:
			if(getsym(cstrings, c, arg[0], arg[1]) == nil)
				goto Error;
			break;
		}
	}

	if(addkey(c, file, proto, sym, flag) < 0)
		goto Error;
	return c;
}

String*
configtostr(Config *c, int unsafe)
{
	int i;
	String *s;

	s = s_reset(nil);

	if(unsafe) {
		for(i=0; i<c->nsym; i++) {
			s_append(s, c->sym[i].name);
			s_append(s, "=\"");
			s_append(s, s_to_c(c->sym[i].val));
			s_append(s, "\"\n");
		}
		s_append(s, "\n");
	}
	for(i=0; i<c->nkey; i++) {
		s_append(s, "file ");
		s_append(s, c->key[i]->file);
		s_append(s, "\nprotocol ");
		s_append(s, c->key[i]->proto->name);
		s_append(s, "\ndata $");
		s_append(s, c->key[i]->sym->name);
		if(c->key[i]->flags & Fconfirmuse)
			s_append(s, "\nflag confirmuse");
		if(c->key[i]->flags & Fconfirmopen)
			s_append(s, "\nflag confirmopen");
		s_append(s, "\n\n");
	}

	return s;
}

void
freeconfig(Config *c)
{
	int i;

	if(c == nil)
		return;

	for(i=0; i<c->nkey; i++) {
		if(c->key[i]) {
			free(c->key[i]->file);
			free(c->key[i]);
		}
		c->key[i] = (Key*)0xDeadBeef;
	}
	free(c->key);
	c->key = (Key**)0xDeadBeef;

	for(i=0; i<c->nsym; i++) {
		free(c->sym[i].name);
		s_free(c->sym[i].val);
		c->sym[i].name = (char*)0xDeadBeef;
		c->sym[i].val = (String*)0xDeadBeef;
	}
	free(c->sym);
	c->sym = (Symbol*)0xDeadBeef;

	free(c);
}

