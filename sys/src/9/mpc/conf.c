#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"init.h"

char*
getconf(char *name)
{
	char **p;
	int n;

	n = strlen(name);

	for(p = conf; *p; p++) {
		if(strncmp(*p, name, n) == 0 && (*p)[n] == '=')
			return *p+n+1;
	}
	return 0;
}

int
isaconfig(char *class, int ctlrno, ISAConf *isa)
{
	char cc[NAMELEN], *p, *q, *r;

	strcpy(cc, class);
	p = cc + strlen(cc);
	*p++ = ctlrno + '0';
	*p = 0;

	p = getconf(cc);
	if(p == 0)
		return 0;

	while(*p){
		while(*p == ' ' || *p == '\t')
			p++;
		if(*p == '\0')
			break;
		if(strncmp(p, "type=", 5) == 0){
			p += 5;
			for(q = isa->type; q < &isa->type[NAMELEN-1]; q++){
				if(*p == '\0' || *p == ' ' || *p == '\t')
					break;
				*q = *p++;
			}
			*q = '\0';
		}
		else if(strncmp(p, "port=", 5) == 0)
			isa->port = strtoul(p+5, &p, 0);
		else if(strncmp(p, "irq=", 4) == 0)
			isa->irq = strtoul(p+4, &p, 0);
		else if(strncmp(p, "mem=", 4) == 0)
			isa->mem = strtoul(p+4, &p, 0);
		else if(strncmp(p, "size=", 5) == 0)
			isa->size = strtoul(p+5, &p, 0);
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
