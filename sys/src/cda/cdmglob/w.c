#include <stdio.h>
#include "cdmglob.h"


void
wcall (Call *c)
{
	Macro	*m = c->c_type;
	int	i;

#ifdef jhc
	extern int	flat_flag, v_flag;
	extern Sig	unconnected;
	char *cp, key;

	key = 'c';
	if (flat_flag) {
		cp = c->c_name;
		if(*m->m_name == '<') { /* connectors are global */
			while(*cp++);
			while((*cp != '/') && (cp >= c->c_name)) cp--;
			cp++;
			key = 'o';
		}
		printf (".%c\t%s\t%s\n", key, cp, m->m_name);
	} else
		printf (".c\t%s\t%s\n", c->c_name, m->m_name);

	for (i = 0; i < c->c_argc; i++) {
		if (c->c_argv[i] == &unconnected)
			continue;
		if (flat_flag && v_flag)
			printf ("\t%s\t%d\t%s\n",
			    c->c_argv[i]->s_name,
			    /*
			     * kludge: the following depends on quirks
			     * in the way signal and pin lookups are
			     * handled for chips.
			     */
			    m->m_pins[i].p_num, m->m_pins[i].p_name);
		else if (flat_flag)
			printf ("\t%s\t%d\n",
			    c->c_argv[i]->s_name,
			    /*
			     * kludge: the following depends on quirks
			     * in the way signal and pin lookups are
			     * handled for chips.
			     */
			    m->m_pins[i].p_num);
		else
			printf ("\t%s\t,%s\n",
			    c->c_argv[i]->s_name, m->m_argv[i]->s_name);
	}
#else
	printf (".c\t%s\t%s\n", c->c_name, m->m_name);

	for (i = 0; i < c->c_argc; i++) {
		printf ("\t%s\t,%s\n",
		    c->c_argv[i]->s_name, m->m_argv[i]->s_name);
	}
#endif

	return;
}


void
whead (Macro *m)
{
	if (m->m_file)
		printf (".f\t%s\n", m->m_file);
	return;
}


void
wtail (Macro *m)
{
	Sig	*s, **ss = m->m_argv;
	Pin	*p = m->m_pins;
	int	n = m->m_argc;

	for (; n > 0; n--, ss++, p++) {
		s = *ss;
		printf (".m\t%s\t%s\t", p->p_name, s->s_name);	/* 2nd arg is pin! bnl 12/11/90 */
		if (s->s_type & S_IN)
			if (s->s_type & S_OUT)
				printf ("<>\n");
			else	printf ("<\n");
		else	if (s->s_type & S_OUT)
				printf (">\n");
			else	printf ("?\n");
	}
	return;
}
