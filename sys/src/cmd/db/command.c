/*
 *
 *	debugger
 *
 */

#include "defs.h"
#include "fns.h"

char	BADEQ[] = "unexpected `='";

BOOL	executing;
extern	char	*lp;

extern	char	lastc, peekc;
char	eqformat[ARB] = "z";
char	stformat[ARB] = "zMi";

ADDR	ditto;

ADDR	dot;
WORD	dotinc;
WORD	adrval, cntval, loopcnt;
int	adrflg, cntflg;
int	adrsp, dotsp, ditsp;

/* command decoding */

command(char *buf, int defcom)
{
	int	modifier, regptr;
	char	savc;
	char	*savlp=lp;
	char	savlc = lastc;
	char	savpc = peekc;
	static char lastcom = '=', savecom = '=';

	if (defcom == 0)
		defcom = lastcom;
	if (buf) {
		if (*buf==EOR)
			return(FALSE);
		clrinp();
		lp=buf;
	}
	do {
		if (adrflg=expr(0)) {
			dot=ditto=expv;
			dotsp=ditsp=expsp;
		}
		adrval=dot;
		adrsp=dotsp;
		if (rdc()==',' && expr(0)) {
			cntflg=TRUE;
			cntval=expv;
		} else {
			cntflg=FALSE;
			cntval=1;
			reread();
		}
		if (!eol(rdc()))
			lastcom=lastc;
		else {
			if (adrflg==0)
				dot=inkdot(dotinc);
			reread();
			lastcom=defcom;
		}
		switch(lastcom) {
		case '/':
		case '=':
		case '?':
			savecom = lastcom;
			acommand(lastcom);
			break;

		case '>':
			lastcom = savecom; 
			savc=rdc();
			if ((regptr=getreg(savc)) != BADREG)
				rput(regptr, dot);
			else if ((modifier=varchk(savc)) != -1)	
				var[modifier]=dot;
			else	
				error("bad variable");
			break;

		case '!':
			lastcom=savecom;
			shell(); 
			break;

		case '$':
			lastcom=savecom;
			printtrace(nextchar()); 
			break;

		case ':':
			if (!executing) { 
				executing=TRUE;
				subpcs(nextchar());
				executing=FALSE;
				lastcom=savecom;
			}
			break;

		case 0:
			prints(DBNAME);
			break;

		default: 
			error("bad command");
		}
		flushbuf();
	} while (rdc()==';');
	if (buf == 0)
		reread();
	else {
		clrinp();
		lp=savlp;
		lastc = savlc;
		peekc = savpc;
	}

	if(adrflg)
		return dot;
	return 1;
}

/*
 * [/?][wml]
 */

void
acommand(int pc)
{
	int itype;
	Map *map;
	int eqcom;
	int star;
	char *fmt;
	
	switch (pc) {
	case '/':
		map = cormap;
		itype = SEGDATA; 
		break;

	case '=':
		map = 0;
		itype = SEGNONE; 
		break;

	case '?':
	default:
		map = symmap;
		itype = SEGANY; 
		break;
	}
	eqcom = FALSE;
	star = FALSE;
	if (pc == '=') {
		eqcom = TRUE;
		fmt = eqformat;
	} else {
		fmt = stformat;
		if (rdc()=='*')
			star = TRUE; 
		else
			reread(); 
		if (star) {
			if (map == symmap)
				itype = SEGDATA;
			else
				itype = SEGTEXT;
		}
		if (adrsp == SEGREGS) {
			map = cormap;
			itype = SEGREGS;
		}
	}
	switch (rdc()) {
	case 'm':
		if (eqcom)
			error(BADEQ); 
		cmdmap(map);
		break;

	case 'L':
	case 'l':
		if (eqcom)
			error(BADEQ); 
		cmdsrc(lastc, map, itype, itype);
		break;

	case 'W':
	case 'w':
		if (eqcom)
			error(BADEQ); 
		cmdwrite(lastc, map, itype);
		break;

	default:
		reread();
		getformat(fmt);
		scanform(cntval,!eqcom,fmt,map, itype,itype);
	}
}

void
cmdsrc(int c, Map *map, int itype, int ptype)
{
	WORD w;
	WORD locval, locmsk;
	ADDR savdot;
	ushort sh;

	if (c == 'L')
		dotinc = 4;
	else
		dotinc = 2;
	savdot=dot;
	expr(1); 
	locval=expv;
	if (expr(0))
		locmsk=expv; 
	else
		locmsk = ~0;
	if (c == 'L') {
		for (;;) {
			if (get4(map, dot, itype, &w) == 0 || mkfault
					||  (w & locmsk) == locval)
				break;
			dot = inkdot(dotinc);
		}
	}
	else {
		for (;;) {
			if (get2(map, dot, itype, &sh) || mkfault
				||  (sh&locmsk)==locval)
					break;
			dot = inkdot(dotinc);
		}
	}
	if (errflg) { 
		dot=savdot; 
		errflg="cannot locate value";
	}
	psymoff((WORD)dot,ptype,"");
}

void
cmdwrite(int wcom, Map *map, int itype)
{
	ADDR savdot;
	char format[2];
	int pass;

	format[0] = wcom == 'w' ? 'x' : 'X';
	format[1] = 0;
	expr(1);
	pass = 0;
	do {
		pass++;  
		savdot=dot;
		exform(1, 1, format, map, itype, itype, pass);
		errflg=0; 
		dot=savdot;
		if (wcom == 'W')
			put4(map, dot, itype, expv);
		else
			put2(map, dot, itype, expv);
		savdot=dot;
		dprint("=%8t"); 
		exform(1, 0, format, map, itype, itype, pass);
		newline();
	} while (expr(0) && errflg==0);
	dot=savdot;
	chkerr();
}

/*
 * collect a register name; return register offset
 * this is not what i'd call a good division of labour
 */

int
getreg(int regnam)
{
	char buf[LINSIZ];
	char *p;
	int c;

	p = buf;
	*p++ = regnam;
	while (isalnum(c = readchar()))
		*p++ = c;
	*p = 0;
	reread();
	return (rname(buf));
}

/*
 * shell escape
 */

void
shell(void)
{
	int	rc, unixpid;
	char *argp = lp;

	while (lastc!=EOR)
		rdc();
	if ((unixpid=fork())==0) {
		*lp=0;
		execl("/bin/rc", "rc", "-c", argp, 0);
		exits("execl");				/* botch */
	} else if (unixpid == -1) {
		error("cannot fork");
	} else {
		mkfault = 0;
		while ((rc = wait(0)) != unixpid){
			if(rc == -1 && mkfault){
				mkfault = 0;
				continue;
			}
			break;
		}
		prints("!"); 
		reread();
	}
}
