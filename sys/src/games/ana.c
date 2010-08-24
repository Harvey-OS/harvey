#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	<ctype.h>

#define	NULL		((void *)0)

#define	WORD_LIST	"/sys/games/lib/anawords"
#define	VOWELS		"aeiouy"
#define	ALPHAS		26
#define	LENLIMIT	64

#define	talloc(t)	salloc(sizeof (t))
#define	MAP(c)		((c) - 'a')

enum
{
	in_fd,
	out_fd,
	err_fd,
};

typedef struct word	word;

struct word
{
	char	*text;
	int	length;
	ulong	mask;
	word	*next;
};

typedef word	*set[LENLIMIT];

typedef struct
{
	int	count[ALPHAS];
	int	length;
	ulong	mask;
}
	target;

int	fixed;
int	maxwords;
set	words;
word	*list[LENLIMIT];

void
error(char *s)
{
	fprint(err_fd, "fatal error: %s\n", s);
	exits("fatal error");
}

void	*
salloc(ulong z)
{
	void	*p;

	if ((p = malloc(z)) == NULL)
		error("ran out of memory");

	return p;
}

void
clean_word(char *s)
{
	while (*s && *s != '\n')
	{
		if (isupper(*s))
			*s = tolower(*s);

		s++;
	}
	if (*s == '\n')
		*s = 0;
}

int
word_ok(word *w)
{
	char	*s;
	int	vowel;

	if (w->length == 0 || w->length >= LENLIMIT)
		return 0;

	if (w->length == 1)
		return strchr("aio", w->text[0]) != NULL;

	vowel = 0;
	s = w->text;

	while (*s != '\0')
	{
		if (!isalpha(*s))
			return 0;

		switch (*s)
		{
		case 'a':
		case 'e':
		case 'i':
		case 'o':
		case 'u':
		case 'y':
			vowel = 1;
		}

		s++;
	}

	return vowel;
}

ulong
str_to_mask(char *s)
{
	ulong	m;

	m = 0;

	while (*s != '\0')
		m |= 1 << MAP(*s++);

	return m;
}

word	*
get_word(Biobuf *bp)
{
	char	*s;
	word	*w;

retry:
	if ((s = Brdline(bp, '\n')) == NULL)
		return NULL;

	clean_word(s);

	w = talloc(word);
	w->length = strlen(s);
	w->text = strcpy(salloc(w->length+1), s);
	w->mask = str_to_mask(s);

	if (!word_ok(w))
	{
		free(w->text);
		free(w);
		goto retry;
	}

	return w;
}

void
build_wordlist(void)
{
	Biobuf	*bp;
	word	*w;
	word	**p;

	bp = Bopen(WORD_LIST, OREAD);
	if (!bp)
	{
		perror(WORD_LIST);
		exits(WORD_LIST);
	}

	while ((w = get_word(bp)) != NULL)
	{
		p = &words[w->length];
		w->next = *p;
		*p = w;
	}
}

void
count_letters(target *t, char *s)
{
	int	i;

	for (i = 0; i < ALPHAS; i++)
		t->count[i] = 0;

	t->length = 0;

	while (*s != '\0')
	{
		t->count[MAP(*s++)]++;
		t->length++;
	}
}

int
contained(word *i, target *t)
{
	int	n;
	char	*v;
	target	it;

	if ((i->mask & t->mask) != i->mask)
		return 0;

	count_letters(&it, i->text);

	for (n = 0; n < ALPHAS; n++)
	{
		if (it.count[n] > t->count[n])
			return 0;
	}

	if (it.length == t->length)
		return 1;

	for (v = VOWELS; *v != '\0'; v++)
	{
		if (t->count[MAP(*v)] > it.count[MAP(*v)])
			return 1;
	}

	return 0;
}

int
prune(set in, int m, target *filter, set out)
{
	word	*i, *o, *t;
	int	n;
	int	nz;

	nz = 0;

	for (n = 1; n < LENLIMIT; n++)
	{
		if (n > m)
		{
			out[n] = NULL;
			continue;
		}

		o = NULL;

		for (i = in[n]; i != NULL; i = i->next)
		{
			if (contained(i, filter))
			{
				t = talloc(word);
				*t = *i;
				t->next = o;
				o = t;
				nz = 1;
			}
		}

		out[n] = o;
	}

	return nz;
}

void
print_set(set s)
{
	word	*w;
	int	n;

	for (n = 1; n < LENLIMIT; n++)
	{
		for (w = s[n]; w != NULL; w = w->next)
			fprint(out_fd, "%s\n", w->text);
	}
}

ulong
target_mask(int c[])
{
	ulong	m;
	int	i;

	m = 0;

	for (i = 0; i < ALPHAS; i++)
	{
		if (c[i] != 0)
			m |= 1 << i;
	}

	return m;
}

void
dump_list(word **e)
{
	word	**p;

	fprint(out_fd, "%d", (int)(e - list + 1));

	for (p = list; p <= e; p++)
		fprint(out_fd, " %s", (*p)->text);

	fprint(out_fd, "\n");
}

void
free_set(set s)
{
	int	i;
	word	*p, *q;

	for (i = 1; i < LENLIMIT; i++)
	{
		for (p = s[i]; p != NULL; p = q)
		{
			q = p->next;
			free(p);
		}
	}
}

void
enumerate(word **p, target *i, set c)
{
	target	t;
	set	o;
	word	*w, *h;
	char	*s;
	int	l, m, b;

	l = p - list;
	b = (i->length + (maxwords - l - 1)) / (maxwords - l);

	for (m = i->length; m >= b; m--)
	{
		h = c[m];

		for (w = h; w != NULL; w = w->next)
		{
			if (i->length == m)
			{
				*p = w;
				dump_list(p);
				continue;
			}

			if (l == maxwords - 1)
				continue;

			t = *i;
			t.length -= m;

			for (s = w->text; *s != '\0'; s++)
				t.count[MAP(*s)]--;

			t.mask = target_mask(t.count);
			c[m] = w->next;

			if (prune(c, m, &t, o))
			{
				*p = w;
				enumerate(p + 1, &t, o);
				free_set(o);
			}
		}

		c[m] = h;
	}
}

void
clean(char *s)
{
	char	*p, *q;

	for (p = s, q = s; *p != '\0'; p++)
	{
		if (islower(*p))
			*q++ = *p;
		else if (isupper(*p))
			*q++ = tolower(*p);
	}

	*q = '\0';
}

void
anagramulate(char *s)
{
	target	t;
	set	subjects;

	clean(s);

	if (fixed)
		maxwords = fixed;
	else if ((maxwords = strlen(s) / 4) < 3)
		maxwords = 3;

	fprint(out_fd, "%s:\n", s);
	t.mask = str_to_mask(s);
	count_letters(&t, s);

	if (!prune(words,t.length, &t, subjects))
		return;

	enumerate(&list[0], &t, subjects);
}

void
set_fixed(char *s)
{
	if ((fixed = atoi(s)) < 1)
		fixed = 1;
}

void
read_words(void)
{
	char	*s;
	Biobuf  b;

	Binit(&b, in_fd, OREAD);
	while ((s = Brdline(&b, '\n')) != NULL)
	{
		s[Blinelen(&b)-1] = '\0';
		if (isdigit(s[0]))
		{
			set_fixed(s);
			fprint(out_fd, "Fixed = %d.\n", fixed);
		}
		else
		{
			anagramulate(s);
			fprint(out_fd, "Done.\n");
		}

	}
}

void
main(int argc, char **argv)
{
	build_wordlist();

	if (argc > 1)
		set_fixed(argv[1]);

	read_words();
	exits(0);
}
