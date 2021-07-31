aggr File
{
	int	id;
	int	seq;
	int	ok;
	int	q0, q1;
	byte	*name;
	byte	*addr;
};

byte	*indexfile = "/mnt/acme/index";

void
error(byte *s)
{
	fprint(2, "%s: %s\n", prog, s);
	exits(s);
}

void
errors(byte *s, byte *t)
{
	fprint(2, "%s: %s %s\n", prog, s, t);
	exits(s);
}

void
rerror(byte *s)
{
	fprint(2, "%s: %s: %r\n", prog, s);
	exits(s);
}

int
nrunes(byte *s, int nb)
{
	int i, n;
	Rune r;

	n = 0;
	for(i=0; i<nb; n++)
		i += chartorune(&r, s+i);
	return n;
}

Biobuf	*index;

(int, File*)
findfile(byte *pat)
{
	byte *line, *blank, blankc, *colon, colonc;
	int n;
	File *f, *t;

	if(index == nil)
		index = Bopen(indexfile, OREAD);
	else
		index->seek(0, 0);
	if(index == nil)
		rerror(indexfile);
	for(colon=pat; *colon && *colon!=':'; colon++)
		;
	colonc = *colon;
	*colon = 0;
	n = 0;
	f = nil;
	while((line=index->rdline('\n')) != nil){
		if(index->linelen() < 5*12)
			rerror("bad index file format");
		line[index->linelen()-1] = 0;
		for(blank=line+5*12; *blank && *blank!=' '; blank++)
			;
		blankc = *blank;
		*blank = 0;
		if(strcmp(line+5*12, pat) == 0){
			/* exact match: take that */
			free(f);	/* should also free t->addr's */
			f = malloc(sizeof f[0]);
			if(f == nil)
				rerror("out of memory");
			f->id = atoi(line);
			f->name = strdup(line+5*12);
			if(colonc)
				f->addr = strdup(colon+1);
			else
				f->addr = strdup(".");
			n = 1;
			break;
		}
		if(strstr(line+5*12, pat) != nil){
			/* partial match: add to list */
			f = realloc(f, (n+1)*sizeof f[0]);
			if(f == nil)
				rerror("out of memory");
			t = &f[n++];
			t->id = atoi(line);
			t->name = strdup(line+5*12);
			if(colonc)
				t->addr = strdup(colon+1);
			else
				t->addr = strdup(".");
		}
		*blank = blankc;
	}
	*colon = colonc;
	return (n, f);
}

int
bscmp(File *a, File *b)
{
	return b->seq - a->seq;
}

int
scmp(File *a, File *b)
{
	return a->seq - b->seq;
}

int
ncmp(File *a, File *b)
{
	return strcmp(a->name, b->name);
}
