#include "stdinc.h"
#include "dat.h"
#include "fns.h"

Index			*mainIndex;
int			paranoid = 1;		/* should verify hashes on disk read */

static ArenaPart	*configArenas(char *file);
static ISect		*configISect(char *file);

int
initVenti(char *file, Config *pconf)
{
	Config conf;

	fmtinstall('V', vtScoreFmt);
	fmtinstall('R', vtErrFmt);

	statsInit();

	if(file == nil){
		setErr(EOk, "no configuration file");
		return 0;
	}
	if(!runConfig(file, &conf)){
		setErr(EOk, "can't initialize venti: %R");
		return 0;
	}
	mainIndex = initIndex(conf.index, conf.sects, conf.nsects);
	if(mainIndex == nil)
		return 0;
	if(pconf)
		*pconf = conf;
	return 1;
}

static int
numOk(char *s)
{
	char *p;

	strtoull(s, &p, 0);
	if(p == s)
		return 0;
	if(*p == 0)
		return 1;
	if(p[1] == 0 && strchr("MmGgKk", *p))
		return 1;
	return 0;
}

/*
 * configs	:
 *		| configs config
 * config	: "isect" filename
 *		| "arenas" filename
 *		| "index" name
 *		| "bcmem" num
 *		| "mem" num
 *		| "icmem" num
 *		| "queuewrites"
 *		| "httpaddr" address
 *		| "addr" address
 *
 * '#' and \n delimate comments
 */
enum
{
	MaxArgs	= 2
};
int
runConfig(char *file, Config *config)
{
	ArenaPart **av;
	ISect **sv;
	IFile f;
	char *s, *line, *flds[MaxArgs + 1];
	int i, ok;

	if(!readIFile(&f, file))
		return 0;
	memset(config, 0, sizeof *config);
	config->mem = 0xFFFFFFFFUL;
	ok = 0;
	line = nil;
	for(;;){
		s = ifileLine(&f);
		if(s == nil){
			ok = 1;
			break;
		}
		line = estrdup(s);
		i = getfields(s, flds, MaxArgs + 1, 1, " \t\r");
		if(i == 2 && strcmp(flds[0], "isect") == 0){
			sv = MKN(ISect*, config->nsects + 1);
			for(i = 0; i < config->nsects; i++)
				sv[i] = config->sects[i];
			free(config->sects);
			config->sects = sv;
			config->sects[config->nsects] = configISect(flds[1]);
			if(config->sects[config->nsects] == nil)
				break;
			config->nsects++;
		}else if(i == 2 && strcmp(flds[0], "arenas") == 0){
			av = MKN(ArenaPart*, config->naparts + 1);
			for(i = 0; i < config->naparts; i++)
				av[i] = config->aparts[i];
			free(config->aparts);
			config->aparts = av;
			config->aparts[config->naparts] = configArenas(flds[1]);
			if(config->aparts[config->naparts] == nil)
				break;
			config->naparts++;
		}else if(i == 2 && strcmp(flds[0], "index") == 0){
			if(!nameOk(flds[1])){
				setErr(EAdmin, "illegal index name %s in config file %s", flds[1], config);
				break;
			}
			if(config->index != nil){
				setErr(EAdmin, "duplicate indices in config file %s", config);
				break;
			}
			config->index = estrdup(flds[1]);
		}else if(i == 2 && strcmp(flds[0], "bcmem") == 0){
			if(!numOk(flds[1])){
				setErr(EAdmin, "illegal size %s in config file %s",
					flds[1], config);
				break;
			}
			if(config->bcmem != 0){
				setErr(EAdmin, "duplicate bcmem lines in config file %s", config);
				break;
			}
			config->bcmem = unittoull(flds[1]);
		}else if(i == 2 && strcmp(flds[0], "mem") == 0){
			if(!numOk(flds[1])){
				setErr(EAdmin, "illegal size %s in config file %s",
					flds[1], config);
				break;
			}
			if(config->mem != 0xFFFFFFFFUL){
				setErr(EAdmin, "duplicate mem lines in config file %s", config);
				break;
			}
			config->mem = unittoull(flds[1]);
		}else if(i == 2 && strcmp(flds[0], "icmem") == 0){
			if(!numOk(flds[1])){
				setErr(EAdmin, "illegal size %s in config file %s",
					flds[1], config);
				break;
			}
			if(config->icmem != 0){
				setErr(EAdmin, "duplicate icmem lines in config file %s", config);
				break;
			}
			config->icmem = unittoull(flds[1]);
		}else if(i == 1 && strcmp(flds[0], "queuewrites") == 0){
			config->queueWrites = 1;
		}else if(i == 2 && strcmp(flds[0], "httpaddr") == 0){
			if(!nameOk(flds[1])){
				setErr(EAdmin, "illegal http address '%s' in configuration file %s", flds[1], config);
				break;
			}
			if(config->haddr){
				setErr(EAdmin, "duplicate httpaddr lines in configuration file %s", config);
				break;
			}
			config->haddr = estrdup(flds[1]);
		}else if(i == 2 && strcmp(flds[0], "addr") == 0){
			if(!nameOk(flds[1])){
				setErr(EAdmin, "illegal venti address '%s' in configuration file %s", flds[1], config);
				break;
			}
			if(config->vaddr){
				setErr(EAdmin, "duplicate addr lines in configuration file %s", config);
				break;
			}
			config->vaddr = estrdup(flds[1]);
		}else{
			setErr(EAdmin, "illegal line '%s' in configuration file %s", line, config);
			break;
		}
		free(line);
		line = nil;
	}
	free(line);
	freeIFile(&f);
	if(!ok){
		free(config->sects);
		config->sects = nil;
		free(config->aparts);
		config->aparts = nil;
	}
	return ok;
}

static ISect*
configISect(char *file)
{
	Part *part;

//	fprint(2, "configure index section in %s\n", file);

	part = initPart(file, 0);
	if(part == nil)
		return 0;
	return initISect(part);
}

static ArenaPart*
configArenas(char *file)
{
	Part *part;

//	fprint(2, "configure arenas in %s\n", file);
	part = initPart(file, 0);
	if(part == nil)
		return 0;
	return initArenaPart(part);
}
