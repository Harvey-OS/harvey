#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>
#include <ndb.h>
#include <ip.h>

Biobuf in;

void
usage(void)
{
	fprint(2, "usage: %s [-x netroot]\n", argv0);
	exits("usage");
}

char *vec[] =
{
	"ip",
	"ipmask",
	"dns",
	"dom",
	"auth",
	"authdom",
	"fs",
	"sys",
	"ipgw",
	"ntp",
};

char *outvec[] =
{
	"ipgw",
	"dns",
	"auth",
	"fs",
	"ntp",
	0
};

enum
{
	Tnone,
	Tip,
	Tdom,
	Tmask,
	Tstring,
};

typedef struct Attribute Attribute;
typedef void (Helper)(Attribute*);

struct Attribute
{
	char *attr;
	char *desc;
	int type;
	int multiple;
	Helper *helper;
};

Helper authdomhelper, authhelper, fshelper;

Attribute expl[] =
{
	{ "ip", "ip address on this interface", Tip, 0, nil },
	{ "ipmask", "ip mask on this interface", Tmask, 0, nil },
	{ "dns", "domain name server", Tdom, 1, nil },
	{ "dom", "domain name", Tdom, 0, nil },
	{ "auth", "authentication server", Tdom, 0, authhelper },
	{ "authdom", "authentication domain", Tstring, 0, authdomhelper },
	{ "fs", "file server", Tdom, 0, fshelper },
	{ "sys", "your system name", Tstring, 0, nil },
	{ "ipgw", "ip gateway on this network", 0, nil },
	{ "ntp", "network time server", Tdom, 0, nil },
	{ 0 },
};

char *authserver;

Ndbtuple*
find(char *attr, Ndbtuple *t1, Ndbtuple *t2, Ndbtuple *t3)
{
	for(; t1; t1 = t1->entry)
		if(strcmp(attr, t1->attr) == 0)
			return t1;
	for(; t2; t2 = t2->entry)
		if(strcmp(attr, t2->attr) == 0)
			return t2;
	for(; t3; t3 = t3->entry)
		if(strcmp(attr, t3->attr) == 0)
			return t3;
	return nil;
}

Ndbtuple*
findnext(Ndbtuple *t, char *attr, Ndbtuple *t1, Ndbtuple *t2, Ndbtuple *t3)
{
	if(t == nil)
		return find(attr, t1, t2, t3);
	for(; t1; t1 = t1->entry)
		if(t1 == t)
			return find(attr, t->entry, t2, t3);
	for(; t2; t2 = t2->entry)
		if(t2 == t)
			return find(attr, t->entry, t3, nil);
	for(; t3; t3 = t3->entry)
		if(t3 == t)
			return find(attr, t->entry, nil, nil);
	return nil;
}

Ndbtuple*
newtuple(char *attr, char *val)
{
	Ndbtuple *nt;

	nt = malloc(sizeof *nt);
	nt->entry = nt->line = nil;
	strncpy(nt->attr, attr, Ndbalen-1);
	nt->attr[Ndbalen-1] = 0;
	strncpy(nt->val, val, Ndbvlen-1);
	nt->attr[Ndbvlen-1] = 0;
	return nt;
}

Ndbtuple*
duptuple(Ndbtuple *t)
{
	Ndbtuple *nt;

	nt = malloc(sizeof *nt);
	nt->entry = nt->line = nil;
	strcpy(nt->val, t->val);
	strcpy(nt->attr, t->attr);
	return nt;
}

Ndbtuple*
concat(Ndbtuple *t1, Ndbtuple *t2)
{
	Ndbtuple *t;

	if(t1 == nil)
		return t2;
	if(t2 == nil)
		return t1;

	t = t1;
	for(; t1->entry != nil; t1 = t1->entry)
		;
	t1->entry = t1->line = t2;
	return t;
}

void
removetuple(Ndbtuple **l, Ndbtuple *nt)
{
	Ndbtuple *t;

	while(*l != nil){
		t = *l;
		if(strcmp(t->attr, nt->attr) == 0
		&& strcmp(t->val, nt->val) == 0){
			*l = t->entry;
			free(t);
		} else
			l = &t->entry;
	}
}

void
help(Attribute *e)
{
	if(e->helper){
		print("--------------------------------\n");
		(*e->helper)(e);
		print("--------------------------------\n");
	}
}

Ndbtuple*
need(int must, char *attr)
{
	char *p;
	uchar ip[IPaddrlen];
	int i;
	Attribute *e;
	Ndbtuple *first, **l, *t;

	e = nil;
	for(i = 0; expl[i].attr != nil; i++){
		if(strcmp(expl[i].attr, attr) == 0){
			e = &expl[i];
			break;
		}
	}
	if(e == nil)
		return nil;

	first = nil;
	l = &first;
	for(;;){
		if(first != nil)
			must = 0;
		print("Enter%s %s (type ? for help%s)? ", first!=nil ? " another" : "",
			e->desc, must!=0 ? "" : ", return to skip");

		p = Brdline(&in, '\n');
		p[Blinelen(&in)-1] = 0;
		while(*p == ' ' || *p == '\t')
			p++;

		if(*p == '?'){
			help(e);
			continue;
		}

		if(*p == 0){
			if(must)
				continue;
			else
				break;
		}
		t = nil;
		switch(e->type){
		case Tip:
			parseip(ip, p);
			if(ipcmp(ip, IPnoaddr) == 0){
				print("!not an IP address\n");
				break;
			}
			t = newtuple(attr, p);
			break;
		case Tdom:
			if(strchr(p, '.') == nil && strchr(p, ':') == nil){
				print("!not an IP address or domain name\n");
				break;
			}
			t = newtuple(attr, p);
			break;
		case Tmask:
			parseipmask(ip, p);
			if(ipcmp(ip, IPnoaddr) == 0){
				print("!not an IP mask\n");
				break;
			}
			t = newtuple(attr, p);
			break;
		case Tstring:
			if(strchr(p, ' ')){
				print("!the string cannot contain spaces\n");
				break;
			}
			t = newtuple(attr, p);
			break;
		}
		if(t != nil){
			*l = t;
			l = &t->entry;
		}
	}

	return first;	
}

void
printnet(Ndbtuple *ndb, Ndbtuple *local, Ndbtuple *added)
{
	uchar ip[IPaddrlen];
	uchar mask[IPaddrlen];
	uchar net[IPaddrlen];
	Ndbtuple *t;
	char **l;

	/* some little calculations */
	t = find("ip", ndb, local, added);
	if(t == nil)
		return;
	parseip(ip, t->val);
	t = find("ipmask", ndb, local, added);
	if(t != nil)
		parseipmask(mask, t->val);
	else
		ipmove(mask, defmask(ip));
	maskip(ip, mask, net);

	/* print out a sample network database */
	print("ipnet=mynet ip=%I ipmask=%M\n", net, mask);
	for(l = outvec; *l; l++){
		t = nil;
		while((t = findnext(t, *l, added, ndb, local)) != nil)
			print("	%s=%s\n", t->attr, t->val);
	}
}

void
printauthdom(Ndbtuple *ndb, Ndbtuple *local, Ndbtuple *added)
{
	Ndbtuple *t, *nt;

	t = find("authdom", ndb, local, added);
	if(t == nil)
		return;
	nt = nil;
	print("authdom=%s\n", t->val);
	while((nt = findnext(nt, "auth", ndb, local, added)) != nil)
		print("\tauth=%s\n", nt->val);
}

void
printhost(Ndbtuple *added)
{
	uchar ip[IPaddrlen];
	Ndbtuple *t;
	char **l;

	t = find("ip", added, nil, nil);
	if(t == nil)
		return;
	parseip(ip, t->val);
	print("ip=%I\n", ip);
	for(l = outvec; *l; l++){
		t = nil;
		while((t = findnext(t, *l, added, nil, nil)) != nil)
			print("	%s=%s\n", t->attr, t->val);
	}
}

void
main(int argc, char **argv)
{
	Ndbs s;
	char *net;
	char ndbfile[128];
	char authdomval[Ndbvlen];
	Ndb *db, *netdb;
	Ndbtuple *ndb, *local, *added, *nt, *t, *xt;

	fmtinstall('I', eipfmt);
	fmtinstall('M', eipfmt);

	db = ndbopen("/lib/ndb/local");
	Binit(&in, 0, OREAD);

	net = "/net";
	ARGBEGIN {
	case 'x':
		net = ARGF();
		if(net == nil)
			usage();
		break;
	} ARGEND;

	/* see what ipconfig knows */
	snprint(ndbfile, sizeof ndbfile, "%s/ndb", net);
	netdb = ndbopen(ndbfile);
	ndb = ndbparse(netdb);
	added = nil;

	/* ask user for ip address */
	t = find("ip", ndb, added, nil);
	if(t == nil){
		t = need(1, "ip");
		added = concat(added, t);
	}
	if(t == nil)
		sysfatal("cannot continue without your ip address");

	/* see what the database knows */
	local = ndbipinfo(db, "ip", t->val, vec, nelem(vec));

	/* get hostname */
	t = find("sys", ndb, local, added);
	if(t == nil){
		t = need(1, "sys");
		added = concat(added, t);
	}

	/* get auth server */
	t = find("auth", ndb, local, added);
	if(t == nil){
		t = need(0, "auth");
		added = concat(added, t);
	}
	if(t != nil){
		authserver = t->val;
		xt = ndbgetval(db, &s, "auth", t->val, "authdom", authdomval);
		for(nt = find("authdom", xt, nil, nil); nt != nil;
		    nt = find("authdom", nt->entry, nil, nil))
			local = concat(local, duptuple(nt));
		ndbfree(xt);
		if(xt == nil){
			t = need(1, "authdom");
			added = concat(added, t);
		}
	}

	/* look for things we need */
	if(find("ipmask", added, ndb, local) == nil){
		t = need(0, "ipmask");
		added = concat(added, t);
	}
	if(find("dns", added, ndb, local) == nil){
		t = need(0, "dns");
		added = concat(added, t);
	}
	if(find("ntp", added, ndb, local) == nil){
		t = need(0, "ntp");
		added = concat(added, t);
	}
	if(find("fs", added, ndb, local) == nil){
		t = need(0, "fs");
		added = concat(added, t);
	}

	/* remove redundancy */
	for(t = local; t != nil; t = t->entry)
		removetuple(&ndb, t);

	print("======================================\n");
	print("recomended additions to /lib/ndb/local\n");
	print("======================================\n");

	if(added != nil)
		printnet(ndb, local, added);
	printauthdom(ndb, local, added);
	printhost(added);

	exits(0);
}

void
authentication(void)
{
	print("\tPlan 9 systems that accept connections from other Plan 9\n");
	print("\tsystems need to authenticate to each other.  A set of secrets\n");
	print("\tand authentication servers to validate those secrets is\n");
	print("\tidentified by a string called an 'authentication domain'.\n");
	print("\tIf you expect anyone to 'cpu' to or 'import' from your machine,\n");
	print("\tyour machine must know its authentication domain and the\n");
	print("\tauthentication server(s) that validate keys for it.\n");
}

void
authdomhelper(Attribute *)
{
	authentication();
	print("\n\tEnter the authentication domain that is served by the\n");
	print("\tauthentication server '%s'.\n", authserver);
}

void
authhelper(Attribute *)
{
	authentication();
	print("\n\tEnter the name or address of the default authentication server\n");
	print("\tto be used by this system.\n");
}

void
fshelper(Attribute *)
{
	print("\tThis is only important if you are going to run diskless from\n");
	print("\ta remote server, i.e., if you wish to answer 'il' or 'tcp' to the\n");
	print("\t'root is from:' boot prompt.\n");
}
