#include "all.h"

struct
{
	RWLock	uidlock;
	char*	uidbuf;
	int	flen;
	int	find;
} uidgc;

int
byuid(void *a1, void *a2)
{
	Uid *u1, *u2;

	u1 = a1;
	u2 = a2;
	return u2->uid - u1->uid;
}

int
byname(void *a1, void *a2)
{
	Uid *u1, *u2;

	u1 = a1;
	u2 = a2;
	return strcmp(uidspace+u2->offset, uidspace+u1->offset);
}

int
fchar(void)
{

	if(uidgc.find >= uidgc.flen) {
		uidgc.find = 0;
		uidgc.flen = con_read(FID2, uidgc.uidbuf, cons.offset, MAXDAT);
		if(uidgc.flen <= 0)
			return 0;
		cons.offset += uidgc.flen;
	}
	return uidgc.uidbuf[uidgc.find++];
}

int
fname(char *name)
{
	int i, c;

	memset(name, 0, NAMELEN);
	for(i=0;; i++) {
		c = fchar();
		switch(c) {
		case ':':
		case '\n':
		case ',':
		case ' ':
		case '#':
		case 0:
			return c;
		}
		if(i < NAMELEN-1)
			name[i] = c;
	}
}

#ifdef sometime
/*
 * file format is
 * uid:name:lead:member,member,...\n
 */
void
read_user(void)
{
	int c;

	if((c=fname(ustr))!=':' || (c=fname(name))!=':' || (c=fname(lead))!=':')
		goto skipline;
	n = atol(ustr);
	if(n == 0)
		goto skipline;
	if(readu){
		o -= strlen(name)+1;
		if(o < 0) {
			cprint("conf.uidspace(%ld) too small\n", conf.uidspace);
			return -1;
		}
		strcpy(uidspace+o, name);
		uid[u].uid = n;
		uid[u].offset = o;
		u++;
		if(u >= conf.nuid) {
			cprint("conf.nuid(%ld) too small\n", conf.nuid);
			goto initu;
		}
	}else{
		o = strtouid1(name);
		if(o == 0 && strcmp(name, "") != 0)
			o = n;
		for(c=0; c<u; c++)
			if(uid[c].uid == n) {
				uid[c].lead = o;
				break;
			}
		while(((c=fname(name))==',' || c=='\n') && name[0]){
work here		
			if(c=='\n')
				break;
		}
	}
	
skipline:
	while(c != '\n')
		fname(ustr);
}
#endif

/*
	-1:adm:adm:
	0:none:adm:
	1:tor:tor:
	10000:sys::
	10001:map:map:
	10002:doc::
	10003:upas:upas:
	10004:font::
	10005:bootes:bootes:
*/

struct {
	int	uid;
	char	*name;
	int	leader;
}
admusers[] = {
	-1,	"adm",	-1,
	 0,	"none",	-1,
	 1,	"tor",	1,
	 2,	"glenda",	2,
	10000,	"sys",	0,
	10001,	"upas",	10001,
	10002,	"bootes",	10002,
	0,	0,	0,
};


void
cmd_user(void)
{
	int c, n, o, u, g, i;
	char name[NAMELEN];

	if(con_clone(FID1, FID2))
		goto ainitu;
	if(con_path(FID2, "/adm/users"))
		goto ainitu;
	if(con_open(FID2, 0)){
		goto ainitu;
	}

	wlock(&uidgc.uidlock);
	uidgc.uidbuf = malloc(MAXDAT);

	memset(uid, 0, conf.nuid * sizeof(*uid));
	memset(uidspace, 0, conf.uidspace * sizeof(*uidspace));
	memset(gidspace, 0, conf.gidspace * sizeof(*gidspace));

	uidgc.flen = 0;
	uidgc.find = 0;
	cons.offset = 0;
	u = 0;
	o = conf.uidspace;

ul1:
	c = fname(name);
	if(c != ':')
		goto uskip;
	n = atol(name);
	if(n == 0)
		goto uskip;
	c = fname(name);
	if(c != ':')
		goto uskip;
	o -= strlen(name)+1;
	if(o < 0) {
		cprint("conf.uidspace(%ld) too small\n", conf.uidspace);
		goto initu;
	}
	strcpy(uidspace+o, name);
	uid[u].uid = n;
	uid[u].offset = o;
	u++;
	if(u >= conf.nuid) {
		cprint("conf.nuid(%ld) too small\n", conf.nuid);
		goto initu;
	}

uskip:
	if(c == '\n')
		goto ul1;
	if(c) {
		c = fname(name);
		goto uskip;
	}
/*	cprint("%d uids read\n", u);/**/
	qsort(uid, u, sizeof(uid[0]), byuid);
	for(c=u-1; c>0; c--)
		if(uid[c].uid == uid[c-1].uid) {
			cprint("duplicate uids: %d\n", uid[c].uid);
			cprint("	%s", uidspace+uid[c].offset);
			cprint(" %s\n", uidspace+uid[c-1].offset);
		}
	qsort(uid, u, sizeof(uid[0]), byname);
	for(c=u-1; c>0; c--)
		if(!strcmp(uidspace+uid[c].offset,
		   uidspace+uid[c-1].offset)) {
			cprint("kfs: duplicate names: %s\n", uidspace+uid[c].offset);
			cprint("	%d", uid[c].uid);
			cprint(" %d\n", uid[c-1].uid);
		}
	if(cons.flags & Fuid)
		for(c=0; c<u; c++)
			cprint("%6d %s\n", uid[c].uid, uidspace+uid[c].offset);

	uidgc.flen = 0;
	uidgc.find = 0;
	cons.offset = 0;
	g = 0;

gl1:
	c = fname(name);
	if(c != ':')
		goto gskip;
	n = atol(name);		/* number */
	if(n == 0)
		goto gskip;
	c = fname(name);	/* name */
	if(c != ':')
		goto gskip;
	c = fname(name);	/* leader */
	if(c != ':')
		goto gskip;
	o = strtouid1(name);
	if(o == 0 && strcmp(name, "") != 0)
		o = n;
	for(c=0; c<u; c++)
		if(uid[c].uid == n) {
			uid[c].lead = o;
			break;
		}
	c = fname(name);	/* list of members */
	if(c != ',' && c != '\n')
		goto gskip;
	if(!name[0])
		goto gskip;
	gidspace[g++] = n;
gl2:
	n = strtouid1(name);
	if(n)
		gidspace[g++] = n;
	if(g >= conf.gidspace-2) {
		cprint("conf.gidspace(%ld) too small\n", conf.gidspace);
		goto initu;
	}
	if(c == '\n')
		goto gl3;
	c = fname(name);
	if(c == ',' || c == '\n')
		goto gl2;
	cprint("gid truncated\n");

gl3:
	gidspace[g++] = 0;

gskip:
	if(c == '\n')
		goto gl1;
	if(c) {
		c = fname(name);
		goto gskip;
	}
	if(cons.flags & Fuid) {
		o = 0;
		for(c=0; c<g; c++) {
			n = gidspace[c];
			if(n == 0) {
				o = 0;
				continue;
			}
			uidtostr1(name, n);
			if(o) {
				if(o > 6) {
					cprint("\n       %s", name);
					o = 1;
				} else
					cprint(" %s", name);
			} else
				cprint("\n%6s", name);
			o++;
		}
		cprint("\n");
	}
	goto out;

ainitu:
	wlock(&uidgc.uidlock);
	uidgc.uidbuf = malloc(MAXDAT);

initu:
	cprint("initializing minimal user table\n");
	memset(uid, 0, conf.nuid * sizeof(*uid));
	memset(uidspace, 0, conf.uidspace * sizeof(*uidspace));
	memset(gidspace, 0, conf.gidspace * sizeof(*gidspace));
	o = conf.uidspace;
	u = 0;

	for(i=0; admusers[i].name; i++){
		o -= strlen(admusers[i].name)+1;
		strcpy(uidspace+o, admusers[i].name);
		uid[u].uid = admusers[i].uid;
		uid[u].lead = admusers[i].leader;
		uid[u].offset = o;
		u++;
	}

out:
	free(uidgc.uidbuf);
	writegroup = strtouid1("write");
	wunlock(&uidgc.uidlock);

}

void
uidtostr(char *name, int id)
{
	rlock(&uidgc.uidlock);
	uidtostr1(name, id);
	runlock(&uidgc.uidlock);
}

void
uidtostr1(char *name, int id)
{
	Uid *u;
	int i;

	if(id == 0){
		strncpy(name, "none", NAMELEN);
		return;
	}
	for(i=0, u=uid; i<conf.nuid; i++,u++) {
		if(u->uid == id) {
			strncpy(name, uidspace+u->offset, NAMELEN);
			return;
		}
	}
	strncpy(name, "none", NAMELEN);
}

int
strtouid(char *s)
{
	int i;

	rlock(&uidgc.uidlock);
	i = strtouid1(s);
	runlock(&uidgc.uidlock);
	return i;
}

int
strtouid1(char *s)
{
	Uid *u;
	int i;

	for(i=0, u=uid; i<conf.nuid; i++,u++)
		if(!strcmp(s, uidspace+u->offset))
			return u->uid;
	return 0;
}

int
ingroup(int u, int g)
{
	short *p;

	if(u == g)
		return 1;
	rlock(&uidgc.uidlock);
	for(p=gidspace; *p;) {
		if(*p != g) {
			for(p++; *p++;)
				;
			continue;
		}
		for(p++; *p; p++)
			if(*p == u) {
				runlock(&uidgc.uidlock);
				return 1;
			}
	}
	runlock(&uidgc.uidlock);
	return 0;
}

int
leadgroup(int ui, int gi)
{
	Uid *u;
	int i;

	rlock(&uidgc.uidlock);
	for(i=0, u=uid; i<conf.nuid; i++,u++) {
		if(u->uid == gi) {
			i = u->lead;
			runlock(&uidgc.uidlock);
			if(i == ui)
				return 1;
			if(i == 0)
				return ingroup(ui, gi);
			return 0;
		}
	}
	runlock(&uidgc.uidlock);
	return 0;
}
