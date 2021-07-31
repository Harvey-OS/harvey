#include	"all.h"
#include	"io.h"

struct
{
	char*	icharp;
	char*	charp;
	int	loc;
	int	error;
	int	newconf;	/* clear befor start */
	int	modconf;	/* write back when done */
	int	nextiter;
	int	lastiter;
	int	ipauthset;
} f;

void
cdiag(char *s, int c1)
{

	f.charp--;
	if(f.error == 0) {
		print("config diag: %s -- <%c>\n", s, c1);
		f.error = 1;
	}
}

int
cnumb(void)
{
	int c, n;

	c = *f.charp++;
	if(c == '<') {
		n = f.nextiter;
		if(n) {
			f.nextiter = n+1;
			if(n >= f.lastiter) {
				f.nextiter = 0;
				f.lastiter = 0;
			}
			for(;;) {
				c = *f.charp++;
				if(c == '>')
					break;
			}
			return n;
		}
		n = cnumb();
		if(*f.charp++ != '-') {
			cdiag("- expected", f.charp[-1]);
			return 0;
		}
		c = cnumb();
		if(*f.charp++ != '>') {
			cdiag("> expected", f.charp[-1]);
			return 0;
		}
		f.nextiter = n+1;
		f.lastiter = c;
		return n;
	}
	if(c < '0' || c > '9') {
		cdiag("number expected", c);
		return 0;
	}
	n = 0;
	while(c >= '0' && c <= '9') {
		n = n*10 + (c-'0');
		c = *f.charp++;
	}
	f.charp--;
	return n;
}

Device
config1(int c)
{
	Device d, dl[200];
	int i, m;

	for(i=0;; i++) {
		dl[i] = config();
		if(f.error)
			goto bad;
		m = *f.charp;
		if(c == '(' && m == ')')
			break;
		if(c == '[' && m == ']')
			break;
	}
	f.charp++;
	if(i == 0)
		return dl[0];

	d.type = Devmcat;
	d.ctrl = 0;
	if(c == '[')
		d.type = Devmlev;
	d.unit = f.loc;
	for(m=0; m<=i; m++)
		cwdevs[f.loc+m] = dl[m];
	f.loc += m;
	d.part = f.loc;
	return d;

bad:
	d.type = Devnone;
	return d;
}

Device
config(void)
{
	int c, m;
	Device d, d1, d2;
	char *icp;

	if(f.error)
		goto bad;

	d.type = 0;
	d.ctrl = 0;
	d.unit = 0;
	d.part = 0;

	c = *f.charp++;
	switch(c) {
	default:
		cdiag("unknown type", c);
		goto bad;

	case '(':	/* (d+) one or multiple cat */
	case '[':	/* [d+] one or multiple interleave */
		d = config1(c);
		break;

	case 'f':	/* fd fake worm */
		d.type = Devfworm;
		d1 = config();
		cwdevs[f.loc] = d1;
		d.part = f.loc;
		f.loc++;
		break;

	case 'w':	/* w[#.]# wren [ctrl] unit [part] */
	case 'r':	/* r[#.]#[.#] worm [ctrl] unit [part] */
		icp = f.charp;
		d.type = Devwren;
		if(c == 'r')
			d.type = Devworm;
		d.ctrl = 0;
		d.unit = cnumb();
		d.part = 0;
		m = *f.charp;
		if(m == '.') {
			f.charp++;
			d.part = cnumb();
			m = *f.charp;
			if(m == '.') {
				f.charp++;
				d.ctrl = d.unit;
				d.unit = d.part;
				d.part = cnumb();
			}
		}
		if(f.nextiter)
			f.charp = icp-1;
		break;

	case 'o':	/* ro part of last cw */
		d.type = Devro;	/* the rest is filled in in devinit */
		break;

	case 'c':	/* cdd cache/worm */
		d.type = Devcw;
		d1 = config();
		d2 = config();
		cwdevs[f.loc+0] = d1;
		cwdevs[f.loc+1] = d2;
		d.part = f.loc;
		f.loc += 2;
		break;

	case 'p':	/* pd#.# partition base% size% */
		d.type = Devpart;
		d1 = config();
		cwdevs[f.loc] = d1;
		d.ctrl = f.loc;
		f.loc++;
		d.unit = cnumb();
		c = *f.charp++;
		if(c != '.')
			cdiag("dot expected", c);
		d.part = cnumb();
		break;
	}
	return d;

bad:
	d.type = Devnone;
	return d;
}

char*
strdup(char *s)
{
	int n;
	char *s1;

	n = strlen(s);
	s1 = ialloc(n+1, 0);
	strcpy(s1, s);
	return s1;
}

Device
iconfig(char *s)
{
	Device d;

	f.error = 0;
	f.icharp = s;
	f.charp = f.icharp;
	d = config();
	if(*f.charp) {
		cdiag("junk on end", *f.charp);
		f.error = 1;
	}
	return d;
}

int
testconfig(char *s)
{
	int savfl;

	savfl = f.loc;
	iconfig(s);
	f.loc = savfl;
	return f.error;
}

void
mergeconf(Iobuf *p)
{
	char word[100];
	char *cp;
	Filsys *fs;

	cp = p->iobuf;
	goto line;

loop:
	if(*cp != '\n')
		goto bad;
	cp++;

line:
	cp = getwd(word, cp);
	if(strcmp(word, "") == 0)
		return;
	if(strcmp(word, "service") == 0) {
		cp = getwd(word, cp);
		if(service[0] == 0)
			strcpy(service, word);
		goto loop;
	}
	if(strcmp(word, "ip") == 0) {
		cp = getwd(word, cp);
		if(!nzip(sysip))
			if(chartoip(sysip, word))
				goto bad;
		goto loop;
	}
	if(strcmp(word, "ipgw") == 0) {
		cp = getwd(word, cp);
		if(!nzip(defgwip))
			if(chartoip(defgwip, word))
				goto bad;
		goto loop;
	}
	if(strcmp(word, "ipauth") == 0) {
		cp = getwd(word, cp);
		if(!f.ipauthset)
			if(chartoip(authip, word))
				goto bad;
		goto loop;
	}
	if(strcmp(word, "ipmask") == 0) {
		cp = getwd(word, cp);
		if(!nzip(defmask))
			if(chartoip(defmask, word))
				goto bad;
		goto loop;
	}
	if(strcmp(word, "filsys") == 0) {
		cp = getwd(word, cp);
		for(fs=filsys; fs->name; fs++)
			if(strcmp(fs->name, word) == 0) {
				if(fs->flags & FEDIT) {
					cp = getwd(word, cp);
					goto loop;
				}
				break;
			}
		fs->name = strdup(word);
		cp = getwd(word, cp);
		fs->conf = strdup(word);
		goto loop;
	}
	goto bad;

bad:
	putbuf(p);
	panic("unknown word in config block: %s", word);
}

void
sysinit(void)
{
	Filsys *fs;
	int i, error;
	Device d;
	Iobuf *p;

	dofilter(&u->time);
	dofilter(&cons.work);
	dofilter(&cons.rate);
	dofilter(&cons.bhit);
	dofilter(&cons.bread);
	dofilter(&cons.brahead);
	dofilter(&cons.binit);
	cons.chan = chaninit(Devcon, 1);

	/*
	 * part 1 -- read the config file
	 */
start:
	f.loc = 0;
	print("config %s\n", nvr.config);
	d = iconfig(nvr.config);
	devinit(d);
	p = getbuf(d, 0, Bread);
	if(p && f.newconf) {
		memset(p->iobuf, 0, RBUFSIZE);
		settag(p, Tconfig, 0);
		p->flags |= Bmod;
	}
	if(!p || checktag(p, Tconfig, 0))
		panic("config io");
	mergeconf(p);
	if(f.modconf) {
		memset(p->iobuf, 0, BUFSIZE);
		p->flags |= Bmod|Bimm;
		if(service[0])
			sprint(strchr(p->iobuf, 0), "service %s\n", service);
		for(fs=filsys; fs->name; fs++)
			if(fs->conf)
				sprint(strchr(p->iobuf, 0),
					"filsys %s %s\n", fs->name, fs->conf);
		sprint(strchr(p->iobuf, 0), "ip %I\n", sysip);
		sprint(strchr(p->iobuf, 0), "ipgw %I\n", defgwip);
		sprint(strchr(p->iobuf, 0), "ipauth %I\n", authip);
		sprint(strchr(p->iobuf, 0), "ipmask %I\n", defmask);
		putbuf(p);
		f.modconf = 0;
		f.newconf = 0;
		print("config block written\n");
		goto start;
	}
	putbuf(p);

	print("service %s\n", service);
	print("ip     %I\n", sysip);
	print("ipgw   %I\n", defgwip);
	print("ipauth %I\n", authip);
	print("ipmask %I\n", defmask);

loop:
	/*
	 * part 2 -- squeeze out the deleted filesystems
	 */
	for(fs=filsys; fs->name; fs++)
		if(fs->conf == 0) {
			for(; fs->name; fs++)
				*fs = *(fs+1);
			goto loop;
		}
	if(filsys[0].name == 0)
		panic("no filsys");

	/*
	 * part 3 -- compile the device expression
	 */
	error = 0;
	for(fs=filsys; fs->name; fs++) {
		print("filsys %s %s\n", fs->name, fs->conf);
		i = f.loc;
		fs->dev = iconfig(fs->conf);
		if(f.error) {
			error = 1;
			continue;
		}
		for(; i<f.loc; i++)
			print("	%2d %D\n", i, cwdevs[i]);
	}
	if(error)
		panic("fs config");

	/*
	 * part 4 -- initialize the devices
	 */
	for(fs=filsys; fs->name; fs++) {
		print("sysinit: %s\n", fs->name);
		if(fs->flags & FREAM)
			devream(fs->dev);
		if(fs->flags & FRECOVER)
			devrecover(fs->dev);
		devinit(fs->dev);
	}
}

void
getline(char *line)
{
	char *p;
	int c;

	p = line;
	for(;;) {
		c = rawchar(0);
		if(c == 0 || c == '\n') {
			*p = 0;
			return;
		}
		if(c == '\b') {
			p--;
			continue;
		}
		*p++ = c;
	}
}

void
arginit(void)
{
	int verb, c;
	char line[300], word[300], *cp;
	uchar localip[Pasize];
	Filsys *fs;
	uchar csum;

	print("nvr read\n");
	nvread(NVRAUTHADDR, &nvr, sizeof(nvr));
	csum = nvcsum(nvr.authkey, sizeof(nvr.authkey));
	if(csum != nvr.authsum) {
		print("\n\n ** NVR key checksum is incorrect  **\n");
		print(" ** set password to allow attaches **\n\n");
		memset(nvr.authkey, 0, sizeof(nvr.authkey));
	}
	csum = nvcsum(nvr.config, sizeof(nvr.config));
	if(csum != nvr.configsum) {
		print("\n\n ** NVR config checksum is incorrect  **\n");
		memset(nvr.config, 0, sizeof(nvr.config));
		goto loop;
	}

	print("for config mode hit a key within 5 seconds\n");
	c = rawchar(5);
	if(c == 0) {
		print("	no config\n");
		return;
	}

loop:
	print("config: ");
	getline(line);
	cp = getwd(word, line);
	if(strcmp(word, "end") == 0)
		return;

/*** temp macros for spindle configuration ***/
	if(strcmp(word, "x0") == 0) {
		strcpy(line, "config p(w0)50.1");
		cp = getwd(word, line);
	}
	if(strcmp(word, "x1") == 0) {
		strcpy(line, "filsys main cp(w0)50.25f[p(w0)0.25p(w0)25.25p(w0)75.25]");
		cp = getwd(word, line);
	}

/*** temp macros for bootes configuration ***/
	if(strcmp(word, "z0") == 0) {
		strcpy(line, "config w1.4.0");
		cp = getwd(word, line);
	}
	if(strcmp(word, "z1") == 0) {
		strcpy(line, "filsys main c[w1.4.0w1.5.0w1.6.0](r0.2.<0-44>r0.2.<50-94>)");
		cp = getwd(word, line);
	}
	if(strcmp(word, "z2") == 0) {
		strcpy(line, "filsys other w1.3.0");
		cp = getwd(word, line);
	}
	if(strcmp(word, "allow") == 0) {
		wstatallow = 1;
		writeallow = 1;
		goto loop;
	}
/*** temp ***/

	if(strcmp(word, "noauth") == 0) {
		noauth = !noauth;
		goto loop;
	}
	if(strcmp(word, "noattach") == 0) {
		noattach = !noattach;
		goto loop;
	}
	if(strcmp(word, "ream") == 0) {
		verb = FREAM;
		goto gfsname;
	}
	if(strcmp(word, "recover") == 0) {
		verb = FRECOVER;
		goto gfsname;
	}
	if(strcmp(word, "filsys") == 0) {
		verb = FEDIT;
		goto gfsname;
	}
	if(strcmp(word, "nvram") == 0) {
		getwd(word, cp);
		if(testconfig(word))
			goto loop;
		c = strlen(word);
		if(c >= sizeof(nvr.config)) {
			print("config string too long\n");
			goto loop;
		}
		memset(nvr.config, 0, sizeof(nvr.config));
		memmove(nvr.config, word, c);
		nvr.configsum = nvcsum(nvr.config, sizeof(nvr.config));
		nvwrite(NVRAUTHADDR, &nvr, sizeof(nvr));
		goto loop;
	}
	if(strcmp(word, "config") == 0) {
		getwd(word, cp);
		if(testconfig(word))
			goto loop;
		c = strlen(word);
		if(c >= sizeof(nvr.config)) {
			print("config string too long\n");
			goto loop;
		}
		memset(nvr.config, 0, sizeof(nvr.config));
		memmove(nvr.config, word, c);
		nvr.configsum = nvcsum(nvr.config, sizeof(nvr.config));
		nvwrite(NVRAUTHADDR, &nvr, sizeof(nvr));
		f.newconf = 1;
		goto loop;
	}
	if(strcmp(word, "service") == 0) {
		getwd(word, cp);
		strcpy(service, word);
		f.modconf = 1;
		goto loop;
	}
	if(strcmp(word, "ip") == 0) {
		verb = 0;
		goto ipname;
	}
	if(strcmp(word, "ipgw") == 0) {
		verb = 1;
		goto ipname;
	}
	if(strcmp(word, "ipauth") == 0) {
		f.ipauthset = 1;
		verb = 2;
		goto ipname;
	}
	if(strcmp(word, "ipmask") == 0) {
		verb = 3;
		goto ipname;
	}
	print("unknown config command\n");
	print("	type end to get out\n");
	goto loop;

ipname:
	getwd(word, cp);
	if(chartoip(localip, word)) {
		print("bad ip address\n");
		goto loop;
	}
	switch(verb) {
	case 0:
		memmove(sysip, localip, sizeof(sysip));
		break;
	case 1:
		memmove(defgwip, localip, sizeof(defgwip));
		break;
	case 2:
		memmove(authip, localip, sizeof(authip));
		break;
	case 3:
		memmove(defmask, localip, sizeof(defmask));
		break;
	}
	f.modconf = 1;
	goto loop;

gfsname:
	cp = getwd(word, cp);
	for(fs=filsys; fs->name; fs++)
		if(strcmp(word, fs->name) == 0)
			goto found;
	memset(fs, 0, sizeof(*fs));
	fs->name = strdup(word);

found:
	switch(verb) {
	case FREAM:
		if(strcmp(fs->name, "main") == 0)
			wstatallow = 1;		/* only set, never reset */
	case FRECOVER:
		fs->flags |= verb;
		goto loop;
	case FEDIT:
		f.modconf = 1;
		getwd(word, cp);
		fs->flags |= verb;
		if(word[0] == 0) {
			fs->conf = 0;
			goto loop;
		}
		if(testconfig(word))
			goto loop;
		fs->conf = strdup(word);
		goto loop;
	}
}
