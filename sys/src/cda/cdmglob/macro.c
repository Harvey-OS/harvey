#include <ctype.h>
#include "cdmglob.h"
#include <stdio.h>

#define DIRSIZ	14	/* should come from <sys/dir.h> */
#define MAXPATH	128
#define	NMACROS	1024
#define NMAS	1024


typedef struct MALIAS {
	char		*ma_name;
	Macro		*ma_macro;
	struct MALIAS	*ma_hash;
} Malias;


extern int	flat_flag, kdm_flag, lsl_flag, mws_flag;

static Macro	macros[NMACROS];
static Malias	*mahash[HASH],
		mas[NMAS];
static int	nmacros,
		nmas;
static Malias *malookup (char *name);


/*
 * qsort pins lexicographically by name.
 */

int
cmppinc (Pin **a, Pin **b)
{
	return (strcmp ((*a)->p_name, (*b)->p_name));
}


/*
 * qsort inputs before outputs before ioputs,
 * then numbered pins before unnumbered pins,
 * then numbered pins by increasing pin number,
 * and unnumbered pins lexicographically by name.
 */

int
cmppinm (Pin *a, Pin *b)
{
	if (a->p_dir != b->p_dir)
		return (a->p_dir - b->p_dir);	/* S_IN < S_OUT < S_IN|S_OUT */

	if (a->p_num)
		if (b->p_num)
			return (a->p_num - b->p_num);
		else	return (-1);
	else	if (b->p_num)
			return (1);
		else	return (strcmp (a->p_name, b->p_name));
}


int
hash (char *s, int size)
{
	register long	h = 0;

	while (*s)
		h += h + *s++;
	if (h < 0)
		h = -h;
	return (h % size);
}


void
macalias (char *name1, char *name2)
{
	static int	n;
	Malias		*ma1, *ma2;

	ma1 = malookup (name1);
	ma2 = malookup (name2);
	if (ma1->ma_macro)
		if (ma2->ma_macro) {
			if (ma1->ma_macro != ma2->ma_macro)
				error ("macro redefined");
		}
		else	ma2->ma_macro = ma1->ma_macro;
	else	if (ma2->ma_macro)
			ma1->ma_macro = ma2->ma_macro;
		else	ma1->ma_macro = ma2->ma_macro = macros - ++n;
	return;
}


/*
 * build formal parameter list for macro.
 */

void
macargv (Macro *m)
{
	Pin	*p = m->m_pins;
	Sig	**ss, **tt;
	int	i, n = m->m_npins;

	if (m->m_use)
		return;
	else	m->m_use = 1;	/* can't add any more pins */

	for (i = m->m_argc = 0; i < m->m_nsigs; i++)
		if (m->m_sigs[i].s_type & (S_IN | S_OUT))
			m->m_argc++;

	if (m->m_argc <= 0)
		return;

	qsort ((char *) p, (unsigned) n, sizeof (Pin), cmppinm);

	/*
	 * build sorted pins list for pinmatch().
	 */
	alloc (m->m_psort, n, Pin *);
	for (i = 0; i < n; i++)
		m->m_psort[i] = p + i;
	qsort ((char *) m->m_psort, (unsigned) n, sizeof (Pin *), cmppinc);

	ss = alloc (m->m_argv, m->m_argc, Sig *);

	for (; n > 0; n--, p++) {
		for (tt = p->p_sigs, i = p->p_nsigs; i > 0; i--, *ss++ = *tt++);
		/* Free (p->p_sigs); */
		p->p_sigs = ss - p->p_nsigs;
	}

	/*
	 * find first output arg for shape instance renaming (ugh!).
	 */
	if (kdm_flag)
		for (m->m_out = -1, i = 0; i < m->m_argc; i++)
			if (m->m_argv[i]->s_type & S_OUT) {
				m->m_out = i;
				break;
			}

	return;
}


void
macheck(void)
{
	Call	*c, **cc;
	Macro	*m = macros;
	int	n = nmacros, ncc;

	for (; n > 0; n--, m++) {
		if (m->m_undef)
			continue;
		if (kdm_flag)
			for (cc = m->m_chash, ncc = HASH; ncc > 0; ncc--, cc++)
				*cc = 0;
		for (c = m->m_calls; c; c = c->c_next)
			if (callcheck (c) && kdm_flag) {
				if (c->c_rename)
					c->c_name = c->c_argv[c->c_type->m_out]
					    ->s_name;
				callrelookup (m, c);
			}
	}
	return;
}


void
macinline (Macro *m)
{
	Call	c;
	char	cname[MAXPATH];

	pushmem();
	c.c_type = m;
	*(c.c_name = cname) = 0;
	alloc (c.c_argv, c.c_argc = m->m_argc, Sig *);
	bcopy ((char *) c.c_argv, (char *) m->m_argv,
	    m->m_argc * sizeof (Sig *));
	callinline (&c);
	popmem();
	return;
}


Macro *
maclookup (char	*name, int new)
{
	static Macro	m0;

	Macro	*m, *m2;
	Malias	*ma;
	int	i;

	ma = malookup (name);
	m2 = ma->ma_macro;
	if (macros <= (m2))
		return (m2);
	if (nmacros >= NMACROS)
		fatal ("too many macros");
	m = macros + nmacros++;
	for (ma = mas; ma < mas + nmas; ma++)
		if (ma->ma_macro == m2)
			ma->ma_macro = m;
	*m = m0;
	m->m_name = strsave (name);
	alloc (m->m_phash, HASH, short);
	alloc (m->m_shash, HASH, short);
	alloc (m->m_chash, HASH, Call *);
	for (i = 0; i < HASH; i++) {
		m->m_phash[i] = m->m_shash[i] = -1;
		m->m_chash[i] = 0;
	}
	if (m->m_undef = !new) {
		error ("%s: macro undefined", name);
		return (0);
	}
	return (m);
}


/*
 * derive macro name from file name:
 * [directory/]macro[.page].w
 */

char *
macname (char *file)
{
	static char	name[1+DIRSIZ];
	char		*s, *t;

	for (s = file; *s; s++);
	if (s-file < 3 || s[-2] != '.' || s[-1] != 'w')
		return (0);
	for (s -= 3; s >= file && *s != '/'; s--);
	for (s++, t = name; *s && *s != '.'; s++)
		if (t < name+DIRSIZ)
			*t++ = *s;
	*t = 0;
	return (name);
}


void
macprint(void)
{
	Call	*c;
	Macro	*m, *mm;
	char	*s;
	int	n, use;

	if (mws_flag)
		for (m = macros, n = nmacros; n > 0; n--, m++)
			for (s = m->m_name; *s; s++)
				if (isupper (*s))
					*s += 'a' - 'A';

	for (m = macros, n = nmacros; n > 0; n--, m++) {
		if (mws_flag) {
			for (mm = macros; mm < m; mm++)
				if (Strsame (m->m_name, mm->m_name))
					break;
			if (mm < m)
				continue;
		}
		if (m->m_undef)
			continue;
		use = m->m_use;
		macargv (m);
		if (!m->m_def || flat_flag && use)
			continue;
		if (lsl_flag)
			lslhead (m);
		else	whead (m);
		if (flat_flag)
			macinline (m);
		else	for (c = m->m_calls; c; c = c->c_next) {

				if (lsl_flag)
					lslcall (c);
				else	wcall (c);
		}
		if (lsl_flag)
			lsltail (m);
		else	wtail (m);
	}
	if (lsl_flag)
		lslflush();
	return;
}


static Malias *
malookup (char *name)
{
	Malias	*ma;
	int	h = hash (name, HASH);

	for (ma = mahash[h]; ma; ma = ma->ma_hash)
		if (Strsame (name, ma->ma_name))
			return (ma);
	if (nmas >= NMAS)
		fatal ("too many macro names");
	ma = mas + nmas++;
	ma->ma_name = strsave (name);
	ma->ma_macro = 0;
	ma->ma_hash = mahash[h];
	mahash[h] = ma;
	return (ma);
}
