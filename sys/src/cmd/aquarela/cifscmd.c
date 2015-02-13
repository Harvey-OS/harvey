/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"
#include <bio.h>

SmbClient *c;
Biobuf bin, bout;
static int verbose = 1;

#define	SEP(c)	(((c)==' ')||((c)=='\t')||((c)=='\n'))

typedef struct Slut {
	int8_t *name;
	int val;
} Slut;

static int8_t *
tokenise(int8_t *s, int8_t **start, int8_t **end)
{
	int8_t *to;
	Rune r;
	int n;

	while(*s && SEP(*s))				/* skip leading white space */
		s++;
	to = *start = s;
	while(*s){
		n = chartorune(&r, s);
		if(SEP(r)){
			if(to != *start)		/* we have data */
				break;
			s += n;				/* null string - keep looking */
			while(*s && SEP(*s))	
				s++;
			to = *start = s;
		} 
		else if(r == '\''){
			s += n;				/* skip leading quote */
			while(*s){
				n = chartorune(&r, s);
				if(r == '\''){
					if(s[1] != '\'')
						break;
					s++;		/* embedded quote */
				}
				while (n--)
					*to++ = *s++;
			}
			if(!*s)				/* no trailing quote */
				break;
			s++;				/* skip trailing quote */ 
		}
		else  {
			while(n--)
				*to++ = *s++;
		}
	}
	*end = to;
	return s;
}

static int
parse(int8_t *s, int8_t *fields[], int nfields)
{
	int c, argc;
	int8_t *start, *end;

	argc = 0;
	c = *s;
	while(c){
		s = tokenise(s, &start, &end);
		c = *s++;
		if(*start == 0)
			break;
		if(argc >= nfields-1)
			return -1;
		*end = 0;
		fields[argc++] = start;
	}
	fields[argc] = 0;
Bprint(&bout, "parse returns %d\n", argc);
	return argc;
}

typedef struct {
	int8_t *name;
	int32_t (*f)(SmbClient *, int, int8_t *[]);
	int connected;
	int8_t *help;
} Cmd;
static Cmd cmd[];

static int32_t
cmdhelp(SmbClient *, int argc, int8_t *argv[])
{
	Cmd *cp;
	int8_t *p;

	if(argc)
		p = argv[0];
	else
		p = 0;
	for (cp = cmd; cp->name; cp++) {
		if (p == 0 || strcmp(p, cp->name) == 0)
			Bprint(&bout, "%s\n", cp->help);
	}
	return 0;
}

static Slut sharemodeslut[] = {
	{ "compatibility", SMB_OPEN_MODE_SHARE_COMPATIBILITY },
	{ "exclusive", SMB_OPEN_MODE_SHARE_EXCLUSIVE },
	{ "denywrite", SMB_OPEN_MODE_SHARE_DENY_WRITE },
	{ "denyread", SMB_OPEN_MODE_SHARE_DENY_READOREXEC },
	{ "denynone", SMB_OPEN_MODE_SHARE_DENY_NONE },
	{ 0 }
};

static Slut openmodeslut[] = {
	{ "oread", OREAD },
	{ "owrite", OWRITE },
	{ "ordwr", ORDWR },
	{ "oexec", OEXEC },
	{ 0 }
};

static int
slut(Slut *s, int8_t *pat)
{
	while (s->name) {
		if (cistrcmp(s->name, pat) == 0)
			return s->val;
		s++;
	}
	Bprint(&bout, "%s unrecognised\n", pat);
	return -1;	
}

static int32_t
cmdopen(SmbClient *c, int argc, int8_t *argv[])
{
	int8_t *errmsg;
	int sm, om;
	int rv;
	uint8_t errclass;
	uint16_t error;
	uint16_t fid, attr;
	uint32_t mtime, size;
	uint16_t accessallowed;

	if (argc != 3) {
		Bprint(&bout, "wrong number of arguments\n");
		return -1;
	}
	sm = slut(sharemodeslut, argv[1]);
	if (sm < 0) 
		return -1;
	om = slut(openmodeslut, argv[2]);
	if (om < 0)
		return -1;
	errmsg = nil;
	rv = smbclientopen(c, (sm << 3) | om, argv[0], &errclass, &error, &fid, &attr, &mtime, &size, &accessallowed, &errmsg);
	if (rv == 0) {
		if (errmsg) {
			print("local error %s\n", errmsg);
			free(errmsg);
			return -1;
		}
		return (errclass << 16) | error;
	}
	print("fid 0x%.4ux attr 0x%.4ux time %ld size %lud accessallowed %ud\n",
		fid, attr, mtime, size, accessallowed);
	return 0;
}

static Cmd cmd[] = {
	{ "help",	cmdhelp,	0, "help" },
	{ "open", cmdopen, 1, "open name sharemode openmode" },
	{ 0, 0 },
};

void
threadmain(int argc, int8_t *argv[])
{
	int8_t *errmsg;
	int ac;
	int8_t *ap, *av[256];
	Cmd *cp;
	int32_t status;

	if (argc > 3) {
		print("usage: cifscmd [to [share]]\n");
		exits("args");
	}
	smbglobalsguess(1);
	errmsg = nil;

	if (Binit(&bin, 0, OREAD) == Beof || Binit(&bout, 1, OWRITE) == Beof) {
		fprint(2, "%s: can't init bio: %r\n", argv0);
		threadexits("Binit");
	}

	if (argc > 1) {
		c = smbconnect(argv[1], argc == 3 ? argv[2] : nil, &errmsg);
		if (c == nil)
			fprint(2, "failed to connect: %s\n", errmsg);
	}
	while (ap = Brdline(&bin, '\n')) {
		ap[Blinelen(&bin) - 1] = 0;
		switch (ac = parse(ap, av, nelem(av))) {
		default:
			for (cp = cmd; cp->name; cp++) {
				if (strcmp(cp->name, av[0]) == 0)
					break;
			}
			if (cp->name == 0) {
				Bprint(&bout, "eh?\n");
				break;
			}
			if (c == 0 && cp->connected) {
				Bprint(&bout, "not currently connected\n");
				break;
			}
			if ((status = (*cp->f)(c, ac - 1, &av[1])) != -1) {
				if(verbose)
					Bprint(&bout, "ok %ld/%ld\n", status >> 16, status & 0xffff);
				break;
			}
			break;

		case -1:
			Bprint(&bout, "eh?\n");
			break;

		case 0:
			break;
		}
		Bflush(&bout);
	}
	threadexits(0);
}
