#include <stdio.h>
#include "cdmglob.h"

#define Putc(x)		if (pbuf >= buf+BUFSIZ) lslflush(); *pbuf++ = (x)
#define Putcc(x,y)	Putc (x); Putc (y)
#define Putccc(x,y,z)	Putc (x); Putc (y); Putc (z)
#define Puts(x)		for (r = (x); *r; r++) { Putc (*r); }

static char	buf[BUFSIZ], *pbuf = buf;


void
lslcall (Call *c)
{
	register Macro	*m = c->c_type;
	register char	*r;
	register int	i, n;

	Puts (m->m_name);
	Putcc (':', '\t');
	Puts (c->c_name);
	Putcc (',', '(');

	for (i = n = 0; i < m->m_argc && !(m->m_argv[i]->s_type & S_OUT); i++) {
		if (n++) {
			Putc (',');
		}
		Puts (c->c_argv[i]->s_name);
	}

	Putccc (')', ',', '(');

	for (n = 0; i < m->m_argc; i++) {
		if (n++) {
			Putc (',');
		}
		Puts (c->c_argv[i]->s_name);
	}

	Putccc (')', ';', '\n');
	return;
}


void
lslflush(void)
{
	if (pbuf > buf)
		(void) write (1, buf, (unsigned) (pbuf - buf));
	pbuf = buf;
	return;
}


void
lslhead (Macro *m)
{
	register char	*r;
	Sig		*s, **ss;
	int		i, n;

	Puts ("\nSUBNETWK:\nCKTNAME: ");
	Puts (m->m_name);
	Putcc (';', '\n');
	
	for (n = 0, ss = m->m_argv, i = m->m_argc; i > 0; i--, ss++)
		if (!((*ss)->s_type & S_OUT)) {
			if (n++) {
				Putcc (',', ' ');
			}
			else	Puts ("INPUTS: ");
			Puts ((*ss)->s_name);
		}
	if (n) {
		Putcc (';', '\n');
	}

	for (n = 0, ss = m->m_argv, i = m->m_argc; i > 0; i--, ss++)
		if ((*ss)->s_type & S_OUT) {
			if (n++) {
				Putcc (',', ' ');
			}
			else	Puts ("OUTPUTS: ");
			Puts ((*ss)->s_name);
		}
	if (n) {
		Putcc (';', '\n');
	}

	for (n = 0, s = m->m_sigs, i = m->m_nsigs; i > 0; i--, s++)
		if (s->s_type & S_GLOB) {
			if (n++) {
				Putcc (',', ' ');
			}
			else	Puts ("COMMON: ");
			Puts (s->s_name);
		}
	if (n) {
		Putcc (';', '\n');
	}

	Putc ('\n');
	return;
}


void
lsltail (Macro *m)
{
	register char	*r;

	Puts ("\n* end of ");
	Puts (m->m_name);
	Putccc (';', '\n', '\n');
	return;
}
