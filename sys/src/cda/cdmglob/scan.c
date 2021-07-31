#include <ctype.h>
#include <stdio.h>
#include "cdmglob.h"

#define WORDC	64



extern char	*ego, *filename;
extern int	kdm_flag, lineno;

Macro	*mcur;

static Macro	*tpm;

static char	buf[BUFSIZ],
		line[BUFSIZ],
		com[BUFSIZ],
		*mfile = "-",
		*wordv[WORDC];

static int	ateof, 
		backup,
		bufn,
		fd,
		nbuf;


int
explode (int nwords)
{
	register char	*s;
	register int	inword = 0, wordc = 0;

	for (s = line; *s; s++)
		if (isspace (*s)) {
			if (inword)
				*s = inword = 0;
		}
		else	if (!inword) {
				inword = 1;
				if (wordc >= WORDC) {
					error ("too many words");
					return (-1);
				}
				wordv[wordc++] = s;
			}

	if (wordc >= nwords)
		return (wordc);
	error ("incomplete line");
	return (-1);
}


int
getline(void)
{
	register char	*s, *t;
	register int	c, comment;

	if (ateof)
		return (0);

	if (backup) {
		backup = 0;
		lineno++;
		return (1);
	}

	for (;;)
		for (s = t = line, comment = 0, com[0] = 0;;) {
			if (bufn >= nbuf) {
				if (0 >= (nbuf = read (fd, buf, sizeof buf))) {
					ateof = 1;
					return (0);
				}
				bufn = 0;
			}
			c = buf[bufn++];
			if (c == '\n') {
				lineno++;
				if (t <= line)
					break;		/* empty line */
				*t = 0;
				com[comment - 1] = 0;
				return (1);
			}
			if (c == '%')
				comment = 1;
			if (comment) {
				com[comment++ - 1] = c;
				continue;
			}
			*s++ = c;
			if (!isspace (c))
				t = s;
		}
}


void
newfile (char *file)
{
	char	*name;

	tpm = 0;
	mfile = strsave (file);
	if (name = macname (file)) {
		mcur = maclookup (name, 1);
		if (mcur->m_dcl)
			error ("%s: macro redefined (from \"%s\")",
			    mcur->m_name, mcur->m_file ? mcur->m_file : "???");
		mcur->m_def = 1;
		if (!mcur->m_file)
			mcur->m_file = mfile;
	}
	else	mcur = 0;
	return;
}


/*
 * scan a pins or .w "file".
 * if "file" is zero, scan standard input.
 */

void
scan (char *file)
{
	ateof = backup = lineno = nbuf = 0;

	if (filename = file)
		if (0 <= (fd = open (filename = strsave (file), 0)))
			newfile (file);
		else {
			error ("can't open");
			return;
		}
	else {
		fd = 0;
		mcur = 0;
	}

	while (getline()) {
		if (line[0] != '.') {
			error ("not a command");
			continue;
		}

		switch (line[1]) {
		case 'c':	/* instantiate macro */
		case 'C':
		case 's':	/* instantiate "library shape" */
		case 'S':
			if (isspace (line[2]))
				scanc();
			break;
		case 'f':	/* file name */
		case 'F':
			if (isspace (line[2]))
				if (0 <= explode (2))
					newfile (wordv[1]);
			break;
		case 'm':	/* formal parameter declaration */
		case 'M':
			if (isspace (line[2]))
				scanm();
			break;
		case 't':	/* pin number declarations */
		case 'T':
			switch (line[2]) {
			case ' ':
			case '\t':
#ifdef jhc
				strcat(line, com);
				puts (line);
#endif
				scant();
				break;
			case 'p':
			case 'P':
				scantp();
				break;
#ifdef jhc
			case 't':
			case 'T':
				puts (line);
				break;
#endif
			}
			break;
		}
	}

	if (fd > 0)			/* yes, multiple stdin scans */
		(void) close (fd);
	return;
}


/*
 * `.[cs] instance-name type-name'
 * `	actual-name [formal-pin-number],[formal-name]'
 *	...
 */

void
scanc(void)
{
	Call	**cc;
	Macro	*m = 0;
	Sig	**ss;
	char	*name = (char *) 0;
	int	ec = 0, kdm, ncc = 0, nss, num;

	kdm = kdm_flag && (line[1] == 's' || line[1] == 'S');
	if (!mcur)
		error ("not a .w file");
	else	if (0 <= explode (3))
			if (m = maclookup (wordv[2], 0)) {
				macargv (m);
				if (kdm && m->m_out < 0)
					error ("no outputs");
				else	ncc = callexpand (m, &cc, wordv[1]);
			}

	while (getline())
		if (line[0] == '.') {
			ungetline();
			break;
		}
		else	if (ncc > 0 &&
			    0 <= explode (2) &&
			    scanpin (wordv[1], &num, &name) &&
			    (nss = sigexpand (mcur, wordv[0], &ss)))
				ec += callargv (cc, ncc, ss, nss, num, name);

	if (kdm && m && !ec)
		for (; ncc > 0; ncc--, cc++)
			(*cc)->c_rename = *(*cc)->c_name == '$';
	return;
}


int
scandir (char *dir)
{
	if (*dir == '<') {
		if (!dir[1])
			return (S_IN);
		else	if (dir[1] == '>' && !dir[2])
				return (S_IN | S_OUT);
	}
	else	if (*dir == '>' && !dir[1])
			return (S_OUT);
	error ("%s: bad direction", dir);
	return (0);
}


/*
 * `.m pin-names signal-names direction'
 */

void
scanm(void)
{
	Sig	**s, **s1;
	char	**pnames;
	int	dir, i, npnames, ns;

	if (!mcur) {
		error ("not a .w file");
		return;
	}

	if (0 > explode (4))
		return;

	if (0 >= (dir = scandir (wordv[3])) ||
	    0 >= (ns = sigexpand (mcur, wordv[2], &s)) ||
	    0 >= (npnames = expand (wordv[1], &pnames, 1)))
		return;

	if (npnames == 1) {
		pinlookup (mcur, pnames[0], s, ns, 0, dir);
		return;
	}

	if (npnames != ns)
		error ("pin/arg count mismatch");
	else	for (i = 0; i < npnames; i++) {
			*alloc (s1, 1, Sig *) = s[i];
			pinlookup (mcur, pnames[i], s1, 1, 0, dir);
		}

	/*Free (s);*/
	return;
}


/*
 * `[pin-number],[pin-name-pattern]'
 */

int
scanpin (char *pname, int *num, char **name)
{
	if (*pname == ',')
		*num = 0;
	else {
		if (!(*num = atoi (pname))) {
			error ("%s: bad pin #", pname);
			return (0);
		}
#ifdef jhc
		return (1);	/* "num" instead of "num,name" */
#endif
	}

	for (; *pname && *pname != ','; pname++);

	if (*pname++ != ',')
		error ("missing comma");
	else	if (*(*name = pname))
			return (1);
		else	if (num)
				return (1);
			else	error ("pin #/name missing");

	return (0);
}


/*
 * `.t macro-name [= macro-name]'
 */

void
scant(void)
{
	int	wordc;

	tpm = 0;
	if (0 > (wordc = explode (2)))
		return;
	if (wordc > 2 && *wordv[2] == '=' && !wordv[2][1]) {
		if (wordc < 4)
			error ("missing name");
		else	(void) macalias (wordv[1], wordv[3]);
		return;
	}
	tpm = maclookup (wordv[1], 1);
	if (tpm->m_def || tpm->m_dcl)
		error ("%s: macro redefined (from \"%s\")", tpm->m_name, 
		    tpm->m_file ? tpm->m_file : "???");
	if (!tpm->m_file)
		tpm->m_file = mfile;
	tpm->m_dcl = 1;
	return;
}


/*
 * `.tp pin-names pin-numbers'
 */

void
scantp(void)
{
	Sig	**s;
	int	i, n, ns, wordc;

	if (!tpm ||
	    0 > (wordc = explode (3)) ||
	    0 >= (ns = sigexpand (tpm, wordv[1], &s)))
		return;
	if (ns != wordc - 2)
		error ("pin count mismatch");
	else	for (i = 0; i < ns; i++)
			if (n = atoi (wordv[2+i]))
				pinlookup (tpm, s[i]->s_name, s+i, 1, n,
				    n < 0 ? S_IN : S_OUT);
			else	error ("%s: bad pin number", wordv[2+i]);
	return;
}


void
ungetline(void)
{
	if (!ateof)
		if (lineno--, backup++)
			fatal ("ungetline");
	return;
}
