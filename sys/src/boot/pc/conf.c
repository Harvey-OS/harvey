#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "dosfs.h"

/*
 * Where configuration info is left for the loaded programme.
 * This will turn into a structure as more is done by the boot loader
 * (e.g. why parse the .ini file twice?).
 * There are XXX bytes available at CONFADDR.
 */
#define BOOTLINELEN	64
#define BOOTARGS	((char*)(CONFADDR+BOOTLINELEN))
#define	BOOTARGSLEN	(4096-0x200-BOOTLINELEN)
#define	MAXCONF		32

static char *confname[MAXCONF];
static char *confval[MAXCONF];
static int nconf;

extern char **ini;

char*
getconf(char *name)
{
	int i, n, nmatch;
	char buf[20];

	nmatch = 0;
	for(i = 0; i < nconf; i++)
		if(cistrcmp(confname[i], name) == 0)
			nmatch++;

	switch(nmatch) {
	default:
		print("\n");
		nmatch = 0;
		for(i = 0; i < nconf; i++)
			if(cistrcmp(confname[i], name) == 0)
				print("%d. %s\n", ++nmatch, confval[i]);
		print("%d. none of the above\n", ++nmatch);
		do {
			getstr(name, buf, sizeof(buf), nil, 0);
			n = atoi(buf);
		} while(n < 1 || n > nmatch);

		for(i = 0; i < nconf; i++)
			if(cistrcmp(confname[i], name) == 0)
				if(--n == 0)
					return confval[i];
		break;

	case 1:
		for(i = 0; i < nconf; i++)
			if(cistrcmp(confname[i], name) == 0)
				return confval[i];
		break;

	case 0:
		break;
	}
	return nil;
}

void
addconf(char *fmt, ...)
{
	donprint(BOOTARGS+strlen(BOOTARGS), BOOTARGS+BOOTARGSLEN, fmt, (&fmt+1));
}

/*
 *  read configuration file
 */
static char inibuf[BOOTARGSLEN];
static char id[8] = "ZORT 0\r\n";

int
dotini(Dos *dos)
{
	Dosfile rc;
	int i, n;
	char *cp, *p, *q, *line[MAXCONF];

	if(dosstat(dos, *ini, &rc) <= 0)
		return -1;

	cp = inibuf;
	*cp = 0;
	n = dosread(&rc, cp, BOOTARGSLEN-1);
	if(n <= 0)
		return -1;

	cp[n] = 0;

	/*
	 * Keep a pristine copy.
	 * We could change this to pass the parsed strings
	 * to the booted programme instead of the raw
	 * string, then it only gets done once.
	 */
	if(strncmp(cp, id, sizeof(id))){
		memmove(BOOTARGS, id, sizeof(id));
		if(n+1+sizeof(id) >= BOOTARGSLEN)
			n -= sizeof(id);
		memmove(BOOTARGS+sizeof(id), cp, n+1);
	}
	else
		memmove(BOOTARGS, cp, n+1);

	/*
	 * Strip out '\r', change '\t' -> ' '.
	 */
	p = cp;
	for(q = cp; *q; q++){
		if(*q == '\r')
			continue;
		if(*q == '\t')
			*q = ' ';
		*p++ = *q;
	}
	*p = 0;
	n = getfields(cp, line, MAXCONF, '\n');
	for(i = 0; i < n; i++){
		cp = strchr(line[i], '=');
		if(cp == 0)
			continue;
		*cp++ = 0;
		if(cp - line[i] >= NAMELEN+1)
			*(line[i]+NAMELEN-1) = 0;
		confname[nconf] = line[i];
		confval[nconf] = cp;
		nconf++;
	}
	return 0;
}

static int
parseether(uchar *to, char *from)
{
	char nip[4];
	char *p;
	int i;

	p = from;
	while(*p == ' ')
		++p;
	for(i = 0; i < 6; i++){
		if(*p == 0)
			return -1;
		nip[0] = *p++;
		if(*p == 0)
			return -1;
		nip[1] = *p++;
		nip[2] = 0;
		to[i] = strtoul(nip, 0, 16);
		if(*p == ':')
			p++;
	}
	return 0;
}

int
isaconfig(char *class, int ctlrno, ISAConf *isa)
{
	char cc[NAMELEN], *p, *q, *r;
	int n;

	sprint(cc, "%s%d", class, ctlrno);
	for(n = 0; n < nconf; n++){
		if(cistrncmp(confname[n], cc, NAMELEN))
			continue;
		isa->nopt = 0;
		p = confval[n];
		while(*p){
			while(*p == ' ' || *p == '\t')
				p++;
			if(*p == '\0')
				break;
			if(cistrncmp(p, "type=", 5) == 0){
				p += 5;
				for(q = isa->type; q < &isa->type[NAMELEN-1]; q++){
					if(*p == '\0' || *p == ' ' || *p == '\t')
						break;
					*q = *p++;
				}
				*q = '\0';
			}
			else if(cistrncmp(p, "port=", 5) == 0)
				isa->port = strtoul(p+5, &p, 0);
			else if(cistrncmp(p, "irq=", 4) == 0)
				isa->irq = strtoul(p+4, &p, 0);
			else if(cistrncmp(p, "mem=", 4) == 0)
				isa->mem = strtoul(p+4, &p, 0);
			else if(cistrncmp(p, "size=", 5) == 0)
				isa->size = strtoul(p+5, &p, 0);
			else if(cistrncmp(p, "ea=", 3) == 0){
				if(parseether(isa->ea, p+3) == -1)
					memset(isa->ea, 0, 6);
			}
			else if(isa->nopt < NISAOPT){
				r = isa->opt[isa->nopt];
				while(*p && *p != ' ' && *p != '\t'){
					*r++ = *p++;
					if(r-isa->opt[isa->nopt] >= ISAOPTLEN-1)
						break;
				}
				*r = '\0';
				isa->nopt++;
			}
			while(*p && *p != ' ' && *p != '\t')
				p++;
		}
		return 1;
	}
	return 0;
}
