#include	<audio.h>

void
main(int argc, char *argv[])
{
	int etype, but, i;
	char *file;
	long so;

	rootbin = "/bin";
	rootaudio = "/lib/audio";
	devaudio = "/dev/audio";
	devvolume = "/dev/volume";

	ARGBEGIN {
	default:
		fprint(2, "usage: agui\n");
		exits("usage");
	case 'm':			/* read and convert agui */
		mflag = 1;
		break;
	} ARGEND

	file = getenv("devaudio");
	if(file)
		devaudio = file;
	file = getenv("devvolume");
	if(file)
		devvolume = file;

	file = getenv("home");
	if(file)
		sprint(markfile, "%s/tmp/agui/%s", file, "%s");

	file = "/lib/audio/rawmap";
	if(argc > 0)
		file = argv[0];
	bin = Iopen(file, OREAD);
	if(bin == 0) {
		fprint(2, "cannot open: %s: %r\n", file);
		exits(0);
	}

	Iseek(bin, 0, 0);
	for(i=0; i<4; i++)
		if(Igetc(bin) != "admp"[i]) {
			fprint(2, "%s: not an audio rawmap\n", file);
			exits(0);
		}
	strings = get4(bin, 0);
	nstrings = get4(bin, strings);
	strings += 4;

	Iseek(bin, strings+4*nstrings, 0);
	for(i=0; i<nelem(bschar); i++)
		bschar[i] = get4(bin, 0);

//	for(i = 0; i < nelem(bschar); i += 2)
//		print("%6ld %6ld\n", bschar[i], bschar[i+1]);

	if(mflag) {
		/* recanonical the mark file */
		i = openmark("def");
		if(i >= 0)
			close(i);
		so = Osearch;

	mnext:
		readmark("inzy", so);
		for(i=0; i<nmark; i++) {
			if(marks[i] == so-1)
				continue;
			if(marks[i] == so+1) {
				so = marks[i];
				goto mnext;
			}
			mark(marks[i]);
		}
		exits(0);
	}

	guiinit();
	einit(Emouse|Ekeyboard|Ewait);
	initwait();

	srand(time(0));

	nabove = 0;
	nbelow = 0;
	oplaying = Onone;

	mkgui();

	but = 0;
	update = 0;
	if(0) {
		random = 1;
		play(Oroot);
	}

	for(;;) {
		etype = eread(Emouse|Ekeyboard|Ewait, &ev);
		if(etype & Emouse) {
			but = ev.mouse.buttons;
			guimouse(ev.mouse);
		}
		if(etype & Ekeyboard) {
			if(resetline) {
				resetline = 0;
				if(ev.kbdc != ' ' && ev.kbdc != '\b' && ev.kbdc != '\n')
					guikbd('U'-'@');
			}
			guikbd(ev.kbdc);
		}
		if(etype & Ewait) {
			if(songstopped(ev.data))
				firstsong();
		}
		if(update && !but) {
			guiupdate(update);
			update = 0;
		}
	}
}

int
openmark(char *suffix)
{
	char file[200];
	int f;

	snprint(file, sizeof(file), markfile, suffix);
	f = open(file, OWRITE);
	if(f < 0)
		f = create(file, OWRITE, 0666);
	if(f < 0) {
		markfile[0] = 0;
		return f;
	}
	seek(f, 0, 2);
	return f;
}

void
initwait(void)
{
	int f;
	char buf[20];

	sprint(buf, "/proc/%d/wait", getpid());
	f = open(buf, OREAD);
	if(f < 0) {
		print("cant open %s %r", buf);
		exits("wait");
	}
        estart(Ewait, f, sizeof(Waitmsg));
}

void
expand(long o, int shuf)
{
	int i;
	long o1;

	update |= Dselect;
	nbelow = 0;

	if(shuf) {
		i = nabove - abovdisp - 1;
		if(i >= 0 && i < nabove) {
			o1 = above[i];
			for(i++; i<nabove; i++)
				above[i-1] = above[i];
			above[nabove-1] = o1;
		}
		for(i=0; i<nabove; i++)
			if(above[i] == o)
				break;
		if(i >= nabove && nabove >= nelem(above))
			i = 0;
		if(i < nabove) {
			for(i++; i<nabove; i++)
				above[i-1] = above[i];
		} else
			nabove++;
		above[nabove-1] = o;
		abovdisp = 0;
	}

	if(isoplaying(o))
		o = oplaying;
	if(isospecial(o)) {
		if(dosearch(o) || doqueue(o)) {
			nbelow = 0;
			for(i=0; i<nsearch; i++)
				if(nbelow < nelem(below))
					below[nbelow++] = search[i];
			return;
		}
		return;
	}

	nbelow = get2(bin, get4(bin, o));
	if(nbelow >= nelem(below))
		nbelow = nelem(below);
	for(i=0; i<nbelow; i++) {
		below[i] = Iseek(bin, 0, 1);
		get4(bin, 0);
		get4(bin, 0);
		gets(bin, 0);
	}
}

void
donowplay(void)
{
	long o;
	int n;

	strcpy(nowplaying, "");
	if(!isonone(oplaying))
		snprint(nowplaying, sizeof(nowplaying), "%d. %s", nqueue+1, tstring(oplaying));
	n = nabove - abovdisp - 1;
	if(n >= 0 && n < nabove) {
		o = above[n];
		if(isoplaying(o) || isoqueue(o))
			expand(o, 1);
	}
	update |= Dcursong;
}

void
firstsong(void)
{
	int n;

	if(nqueue == 0) {
		oplaying = Onone;
		donowplay();
		return;
	}

	n = 0;
	if(random)
		n = nrand(nqueue);

	oplaying = queue[n];
	nqueue--;
	memmove(&queue[n], &queue[n+1], (nqueue-n)*sizeof(queue[0]));

	donowplay();
	gets(bin, oplaying+8);		// position to path
	playsong(rootbin, rootaudio, gets(bin, 0));
}

void
play1(long o)
{
	int i, n;
	long m;

	if(isoplaying(o))
		o = oplaying;
	if(isospecial(o)) {
		if(dosearch(o)) {
			for(i=0; i<nsearch; i++) {
				m = search[i];
				if(m != o-1)
					play1(m);
			}
		}
		return;
	}

	o = get4(bin, o+4);
	n = get2(bin, o);

	for(i=0; i<n; i++) {
		o = get4(bin, 0);
		if(nqueue >= nelem(queue)) {
			nqueue++;
			if(random && nrand(nqueue) < nelem(queue))
				queue[nrand(nelem(queue))] = o;
		} else
			queue[nqueue++] = o;
	}
}

void
play(long o)
{

	if(now)
		skipsong(1);

	play1(o);
	if(nqueue > nelem(queue))
		nqueue = nelem(queue);

	if(isonone(oplaying))
		firstsong();
	else
		donowplay();
}

char*
scond(char *s, char *t)
{
	int n;

	n = strlen(t);
	if(memcmp(s, t, n) != 0)
		return 0;
	s += n;
	while(*s == ' ')
		s++;
	if(*s == 0)
		return 0;
	return s;

//	for(t=s; *t; t++)
//		if(*t < '0' || *t > '9')
//			return s;
//	return 0;
}

void
mmap(int f, long o)
{
	int n, i, p, any;
	char *s, *t;
	long m;

	n = get2(bin, o);
	for(p=0; p<4; p++) {
		any = 0;
		m = o+10;
		for(i=0; i<n; i++) {
			s = gets(bin, m);
			m = Iseek(bin, 0, 1) + 8;
			switch(p) {
			case 0:	/* look for title */
				if(t = scond(s, "title")) {
					if(any)
						fprint(f, "	title: %s\n", t);
					else
						fprint(f, "title: %s\n", t);
					any++;
				}
				break;
			case 1:	/* look for artists */
				if(t = scond(s, "artist")) {
					fprint(f, "	artist: %s\n", t);
					any++;
				}
				break;
			case 2:	/* look for other */
				if(t = scond(s, "class")) {
					fprint(f, "	class: %s\n", t);
					any++;
				}
				break;
			case 3:	/* look for path */
				if(t = scond(s, "path")) {
					fprint(f, "	file: %s\n", t);
					any++;
				}
				break;
			}
		}
		if(p == 3 && !any)
			fprint(f, "	file: none\n");
	}
}

void
mark(long o)
{
	int i, n, f;
	long m;

	if(markfile[0] == 0)
		return;

	if(isoplaying(o))
		o = oplaying;
	if(isospecial(o)) {
		if(dosearch(o)) {
			for(i=0; i<nsearch; i++) {
				m = search[i];
				if(m != o-1)
					mark(m);
			}
		}
		return;
	}

	o = get4(bin, o+4);
	n = get2(bin, o);
	o += 2;

	f = openmark("def");
	if(f < 0)
		return;

	for(i=0; i<n; i++) {
		m = get4(bin, o);
		o += 4;
		mmap(f, get4(bin, m));
	}
	close(f);
}

void
skipsong(int flag)
{

	if(flag)	/* stop/skip */
		nqueue = 0;
	stopsong();
}

void
doexit(void)
{
	skipsong(1);
	exits(0);
}

char*
tstring(long o)
{

	if(isospecial(o)) {
		if(isomissing(o))
			return "missing>";
		if(isoplaying(o))
			return "playing>";
		if(isoqueue(o))
			return queuename(o);
		if(isosearch(o))
			return searchname(o);
		return "";
	}
	return gets(bin, o+8);
}

typedef	struct	Head	Head;
struct	Head
{
	int	m;
	long	o;
	long	h;
};

int
wcmp(void *a1, void *a2)
{
	Head *h1, *h2;

	h1 = a1;
	h2 = a2;
	return h1->m - h2->m;
}

void
lookup(char *s, long so)
{
	Head w[10];
	long o, h, h0;
	int i, n, m, c, nskip;
	char *p;


	nsearch = 0;
	if(s == 0)
		return;

	i = (so - Osearch) % Npage;	// page number
	nskip = i * (Pagesize-2);
	if(i > 0) {
		nskip++;
		search[nsearch++] = so-1;
	}

	for(n=0; n<nelem(w); n++) {
		while(*s == ' ')
			s++;
		if(*s == 0)
			break;
		for(p=s; *p != ' ' && *p != 0; p++)
			;
		c = *p;
		*p = 0;
		o = bsearch(s);
		*p = c;
		if(o == 0)
			goto out;
		w[n].m = get2(bin, o);
		w[n].h = 0;
		w[n].o = o+2;
		s = p;
	}
	if(n <= 0)
		goto out;
	qsort(w, n, sizeof(w[0]), wcmp);
	h0 = 1;

loop:
	for(i=0; i<n; i++) {
		h = w[i].h;
		if(h0 > h) {
			m = w[i].m;
			o = w[i].o;
			while(h0 > h) {
				m--;
				if(m < 0)
					goto out;
				h = get4(bin, o);
				o += 4;
			}
			w[i].m = m;
			w[i].o = o;
			w[i].h = h;
		}
		if(h0 < h) {
			h0 = h;
			goto loop;
		}
	}
	if(nskip > 0) {
		nskip--;
		h0++;
		goto loop;
	}

	if(nsearch >= nelem(search)) {
		search[nelem(search)-1] = so+1;
		goto out;
	}
	search[nsearch++] = h0;
	h0++;
	goto loop;

out:;
}

int
doqueue(long o)
{
	int i, nskip;

	nsearch = 0;
	if(!isoqueue(o))
		return 0;

	i = (o - Oqueue);		// page number
	nskip = i * (Pagesize-2);
	if(i > 0) {
		nskip++;
		search[nsearch++] = o-1;
	}

	if(!isonone(oplaying)) {
		if(nskip > 0)
			nskip--;
		else
			search[nsearch++] = oplaying;
	}
	for(i=0; i<nqueue; i++) {
		if(nsearch >= nelem(search)) {
			search[nelem(marks)-1] = o+1;
			goto out;
		}
		if(nskip > 0) {
			nskip--;
			continue;
		}
		search[nsearch++] = queue[i];
	}
out:;
	return 1;
}


int
dosearch(long o)
{
	char *s;
	int i;

	if(!isosearch(o))
		return 0;

	i = (o - Osearch) / Npage;	// search number
	s = searches[i];
	if(s == nil)
		return 0;
	if(s[0] == '.') {
		readmark((s[1]==0)? "def": s+1, o);
		for(i=0; i<nmark; i++)
			search[i] = marks[i];
		nsearch = nmark;
		return 1;
	}
	lookup(s, o);
	return 1;
}

char*
queuename(long o)
{
	char *s;
	int i;

	i = (o - Oqueue);		// search number
	s = gets(bin, Oroot+8);		// just to get static buffer
	sprint(s, "queue.%d>", i);
	return s;
}

char*
searchname(long o)
{
	char *s;
	int i;

	i = (o - Osearch) / Npage;	// search number
	s = searches[i];
	if(s == nil)
		return "gok";
	s = gets(bin, Oroot+8);		// just to get static buffer
	sprint(s, "%s.%ld>", searches[i], (o - Osearch) % Npage);
	return s;
}

void
execute(char *s)
{
	int i;

	i = searchno % Nsearch;
	if(searches[i])
		free(searches[i]);
	searches[i] = strdup(s);
	searchno++;
	expand(Osearch + i*Npage, 1);
}

int
getc(Fio *f)
{
	int c, n;

	if(f->p >= f->ep) {
		n = read(f->f, f->buf, sizeof(f->buf));
		if(n <= 0)
			return -1;
		f->p = f->buf;
		f->ep = f->p + n;
	}
	c = *f->p & 0xff;
	f->p++;
	return c;
}

void
readmark(char *suffix, long so)
{
	int c, i, nskip;
	Fio f;
	char file[200];

	nmark = 0;
	if(markfile[0] == 0)
		return;
	snprint(file, sizeof(file), markfile, suffix);
	f.f = open(file, OREAD);
	if(f.f < 0)
		return;

	i = (so - Osearch) % Npage;	// page number
	nskip = i * (Pagesize-2);
	if(i > 0) {
		nskip++;
		marks[nmark++] = so-1;
	}
	f.p = 0;
	f.ep = 0;

	c = 0;

l0:
	while(c != 'f') {
		if(c < 0)
			goto out;
		c = getc(&f);
	}
	for(i=0; i<4; i++) {
		c = getc(&f);
		if(c != "ile:"[i])
			goto l0;
	}

	i = 0;
	for(;;) {
		c = getc(&f);
		if(c < 0)
			goto out;
		if(c == ' ' || c == '\t')
			continue;
		if(c == '\n')
			break;
		file[i] = c;
		if(i < nelem(file)-5)
			i++;
	}
	if(nskip > 0) {
		nskip--;
		goto l0;
	}
	file[i] = 0;
	lookup(file, Osearch);
	if(nsearch == 0 && nmark < nelem(marks))
		marks[nmark++] = Omissing;		// shouldnt happen
	for(i=0; i<nsearch; i++) {
		if(nmark >= nelem(marks)) {
			marks[nelem(marks)-1] = so+1;
			goto out;
		}
		marks[nmark++] = search[i];
	}
	goto l0;

out:
	close(f.f);
}
