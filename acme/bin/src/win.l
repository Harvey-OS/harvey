#include <alef.h>

#define	EVENTSIZE	256

aggr Event
{
	int	c1;
	int	c2;
	int	q0;
	int	q1;
	int	flag;
	int	nb;
	int	nr;
	byte	b[EVENTSIZE*UTFmax+1];
	Rune	r[EVENTSIZE+1];
};

Event blank = {
	'M',
	'X',
	0, 0, 0, 1, 1,
	{ ' ', 0 },
	{ ' ', 0 },
};

void	main1(int, byte**);
void	error(byte*);
void	run(byte**, int, chan(int));
void	stdin(int, int, int, int, int);
void	stdout(int, int, int);
void	type(Event*, int, int, int);
void	send(Event*, int, int, int, int, int);
byte	*onestring(int, byte**);
int	delete(Event*);
void	deltype(uint, uint);

int	pid;
int	pgrpfd;
int	parentpid;

byte	*typing;
int	ntypeb;
int	ntyper;
int	ntypebreak;

aggr Q
{
	QLock;
	int		p;
	int		k;
};

Q	q;

byte *rc[] = { "rc", "-i", nil };

void
main(int argc, byte **argv)
{
	proc main1(argc, argv);
	exits(nil);
}


void
main1(int argc, byte **argv)
{
	byte **program;
	int i, fd, ctlfd, eventfd, addrfd, datafd, id;
	chan(int) c;
	byte buf[256], buf1[256], buf2[64], *name;

	rfork(RFNAMEG|RFNOTEG);
	parentpid = getpid();
	program = nil;
	if(argc > 1)
		program = argv+1;
	name = nil;
	if(program == nil){
		program = rc;
		name = getenv("sysname");
	}
	if(name == nil){
		name = utfrrune(program[0], '/');
		if(name)
			name++;
		else
			name = program[0];
		strcpy(buf2, name);
		for(i=2; i<argc && strlen(buf2)+1+strlen(argv[i])<16; i++){
			strcat(buf2, "_");
			strcat(buf2, argv[i]);
		}
		name = buf2;
	}
	if(bind("#|", "/dev/acme", MREPL) < 0)
		error("pipe");
	ctlfd = open("/mnt/8½/new/ctl", ORDWR|OCEXEC);
	if(ctlfd<0 || read(ctlfd, buf, 12)!=12)
		error("ctl");
	id = atoi(buf);
	sprint(buf, "/mnt/8½/%d/tag", id);
	fd = open(buf, OWRITE|OCEXEC);
	write(fd, " Send Delete", 12);
	close(fd);
	sprint(buf, "/mnt/8½/%d/event", id);
	eventfd = open(buf, ORDWR|OCEXEC);
	sprint(buf, "/mnt/8½/%d/addr", id);
	addrfd = open(buf, ORDWR|OCEXEC);
	sprint(buf, "/mnt/8½/%d/data", id);
	datafd = open(buf, ORDWR|OCEXEC);
	if(eventfd<0 || addrfd<0 || datafd<0)
		error("data files");
	alloc c;
	proc run(program, id, c);
	pid = <-c;
	sprint(buf, "/proc/%d/notepg", pid);
	pgrpfd = open(buf, OWRITE|OCEXEC);
	if(pgrpfd < 0)
		print("warning: win can't open notepg: %r\n");
	c <-= 1;
	fd = open("/dev/acme/data", ORDWR);
	if(fd < 0)
		error("/dev/acme/data");
	getwd(buf1, sizeof buf1);
	sprint(buf, "name %s/-%s\n0\n", buf1, name);
	write(ctlfd, buf, strlen(buf));
	sprint(buf, "dumpdir %s/\n", buf1);
	write(ctlfd, buf, strlen(buf));
	sprint(buf, "dump %s\n", onestring(argc, argv));
	write(ctlfd, buf, strlen(buf));
	proc stdin(fd, ctlfd, eventfd, addrfd, datafd);
	stdout(fd, addrfd, datafd);
}

void
run(byte **argv, int id, chan(int) c)
{
	byte buf[256];
	int fd0, fd1;

	rfork(RFENVG|RFFDG|RFNOTEG|RFMEM);
	c <-= getpid();
	<-c;
	rfork(RFNAMEG);
	if(bind("/dev/acme/data1", "/dev/cons", MREPL) < 0){
		fprint(2, "can't bind /dev/cons: %r\n");
		exits("bind");
	}
	fd0 = open("/dev/cons", OREAD);
	fd1 = open("/dev/cons", OWRITE);
	if(fd0<0 || fd1<0){
		fprint(2, "can't open /dev/cons: %r\n");
		exits("open");
	}
	dup(fd0, 0);
	dup(fd1, 1);
	dup(fd1, 2);
	close(fd0);
	close(fd1);
	sprint(buf, "/mnt/8½/%d", id);
	if(bind(buf, "/dev/acme", MREPL) < 0)
		error("bind /dev/acme");
	exec(argv[0], argv);
	sprint(buf, "/bin/%s", argv[0]);
	exec(buf, argv);
	exits("can't exec");
}

void
error(byte *s)
{
	if(s)
		fprint(2, "win: %s: %r\n", s);
	else
		s = "kill";
	if(pid)
		write(pgrpfd, "hangup", 6);
	postnote(PNGROUP, parentpid, s);
	exits(s);
}

byte buf[8192];
byte *bufp;
int nbuf;

byte*
onestring(int argc, byte **argv)
{
	byte *p;
	int i, n;

	if(argc == 0)
		return "";
	p = buf;
	for(i=0; i<argc; i++){
		n = strlen(argv[i]);
		if(p+n+1 >= buf+sizeof buf)
			break;
		memmove(p, argv[i], n);
		p += n;
		*p++ = ' ';
	}
	p[-1] = 0;
	return buf;
}

int
getec(int efd)
{
	if(nbuf == 0){
		nbuf = read(efd, buf, sizeof buf);
		if(nbuf <= 0)
			error(nil);
		bufp = buf;
	}
	--nbuf;
	return *bufp++;
}

int
geten(int efd)
{
	int n, c;

	n = 0;
	while('0'<=(c=getec(efd)) && c<='9')
		n = n*10+(c-'0');
	if(c != ' ')
		error("event number syntax");
	return n;
}

int
geter(int efd, byte *buf, int *nb)
{
	Rune r;
	int n;

	r = getec(efd);
	buf[0] = r;
	n = 1;
	if(r < Runeself)
		goto Return;
	while(!fullrune(buf, n))
		buf[n++] = getec(efd);
	chartorune(&r, buf);
    Return:
	*nb = n;
	return r;
}

void
gete(int efd, Event *e)
{
	int i, nb;

	e->c1 = getec(efd);
	e->c2 = getec(efd);
	e->q0 = geten(efd);
	e->q1 = geten(efd);
	e->flag = geten(efd);
	e->nr = geten(efd);
	if(e->nr > EVENTSIZE)
		error("event string too long");
	e->nb = 0;
	for(i=0; i<e->nr; i++){
		e->r[i] = geter(efd, e->b+e->nb, &nb);
		e->nb += nb;
	}
	e->r[e->nr] = 0;
	e->b[e->nb] = 0;
	if(getec(efd) != '\n')
		error("event syntax 2");
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

void
stdin(int fd0, int cfd, int efd, int afd, int dfd)
{
	Event e, e2, e3, e4;

	for(;;){
		gete(efd, &e);
		q.lock();
		switch(e.c1){
		default:
		Unknown:
			print("unknown message %c%c\n", e.c1, e.c2);
			break;

		case 'E':	/* write to body; can't affect us */
			break;

		case 'F':	/* generated by our actions; ignore */
			break;

		case 'K':
		case 'M':
			switch(e.c2){
			case 'I':
				if(e.q0 < q.p)
					q.p += e.q1-e.q0;
				else if(e.q0 <= q.p+ntyper)
					type(&e, fd0, afd, dfd);
				break;

			case 'D':
				q.p -= delete(&e);
				break;

			case 'x':
			case 'X':
				if(e.flag & 2)
					gete(efd, &e2);
				if(e.flag & 8){
					gete(efd, &e3);
					gete(efd, &e4);
				}
				if(e.flag&1 || (e.c2=='x' && e.nr==0 && e2.nr==0)){
					/* send it straight back */
					fprint(efd, "%c%c%d %d\n", e.c1, e.c2, e.q0, e.q1);
					break;
				}
				if(e.q0==e.q1 && (e.flag&2)){
					e2.flag = e.flag;
					e = e2;
				}
				if(e.flag & 8){
					if(e.q1 != e.q0){
						send(&e, fd0, cfd, afd, dfd, 0);
						send(&blank, fd0, cfd, afd, dfd, 0);
					}
					send(&e3, fd0, cfd, afd, dfd, 1);
				}else	 if(e.q1 != e.q0)
					send(&e, fd0, cfd, afd, dfd, 1);
				break;

			case 'l':
			case 'L':
				/* just send it back */
				if(e.flag & 2)
					gete(efd, &e2);
				fprint(efd, "%c%c%d %d\n", e.c1, e.c2, e.q0, e.q1);
				break;

			case 'd':
			case 'i':
				break;

			default:
				goto Unknown;
			}
		}
		q.unlock();
	}
}

void
stdout(int fd1, int afd, int dfd)
{
	int n, m, w, npart;
	byte *buf, *s, *t;
	Rune r;
	byte x[16], hold[UTFmax];

	buf = malloc(8192+UTFmax+1);
	npart = 0;
	for(;;){
		n = read(fd1, buf+npart, 8192);
		if(n < 0)
			error(nil);
		if(n == 0)
			continue;

		/* squash NULs */
		s = memchr(buf+npart, 0, n);
		if(s){
			for(t=s; s<buf+npart+n; s++)
				if(*t = *s)	/* assign = */
					t++;
			n = t-(buf+npart);
		}

		n += npart;

		/* hold on to final partial rune */
		npart = 0;
		while(n>0 && (buf[n-1]&0xC0)){
			--n;
			npart++;
			if((buf[n]&0xC0)!=0x80){
				if(fullrune(buf+n, npart)){
					w = chartorune(&r, buf+n);
					n += w;
					npart -= w;
				}
				break;
			}
		}
		if(n > 0){
			memmove(hold, buf+n, npart);
			buf[n] = 0;
			q.lock();
			m = sprint(x, "#%d", q.p);
			if(write(afd, x, m) != m)
				error("stdout writing address");
			if(write(dfd, buf, n) != n)
				error("stdout writing body");
			q.p += nrunes(buf, n);
			q.unlock();
			memmove(buf, hold, npart);
		}
	}
}

int
delete(Event *e)
{
	uint q0, q1;
	int deltap;

	q0 = e->q0;
	q1 = e->q1;
	if(q1 <= q.p)
		return e->q1-e->q0;
	if(q0 >= q.p+ntyper)
		return 0;
	deltap = 0;
	if(q0 < q.p){
		deltap = q.p-q0;
		q0 = 0;
	}else
		q0 -= q.p;
	if(q1 > q.p+ntyper)
		q1 = ntyper;
	else
		q1 -= q.p;
	deltype(q0, q1);
	return deltap;
}

void
addtype(int c, uint p0, byte *b, int nb, int nr)
{
	int i, w;
	Rune r;
	uint p;
	byte *b0;

	for(i=0; i<nb; i+=w){
		w = chartorune(&r, b+i);
		if(r==0x7F && c=='K'){
			write(pgrpfd, "interrupt", 9);
			/* toss all typing */
			q.p += ntyper+nr;
			ntypebreak = 0;
			ntypeb = 0;
			ntyper = 0;
			/* buglet:  more than one delete ignored */
			return;
		}
		if(r=='\n' || r==0x04)
			ntypebreak++;
	}
	typing = realloc(typing, ntypeb+nb);
	if(typing == nil)
		error("realloc");
	if(p0 == ntyper)
		memmove(typing+ntypeb, b, nb);
	else{
		b0 = typing;
		for(p=0; p<p0 && b0<typing+ntypeb; p++){
			w = chartorune(&r, b0+i);
			b0 += w;
		}
		if(p != p0)
			error("typing: findrune");
		memmove(b0+nb, b0, (typing+ntypeb)-b0);
		memmove(b0, b, nb);
	}
	ntypeb += nb;
	ntyper += nr;
}

void
sendtype(int fd0)
{
	int i, n, nr;

	while(ntypebreak){
		for(i=0; i<ntypeb; i++)
			if(typing[i]=='\n' || typing[i]==0x04){
				n = i + (typing[i] == '\n');
				i++;
				if(write(fd0, typing, n) != n)
					error("sending to program");
				nr = nrunes(typing, i);
				q.p += nr;
				ntyper -= nr;
				ntypeb -= i;
				memmove(typing, typing+i, ntypeb);
				ntypebreak--;
				continue 2;
			}
		print("no breakchar\n");
		ntypebreak = 0;
	}
}

void
deltype(uint p0, uint p1)
{
	int w;
	uint p, b0, b1;
	Rune r;

	/* advance to p0 */
	b0 = 0;
	for(p=0; p<p0 && b0<ntypeb; p++){
		w = chartorune(&r, typing+b0);
		b0 += w;
	}
	if(p != p0)
		error("deltype 1");
	/* advance to p1 */
	b1 = b0;
	for(; p<p1 && b1<ntypeb; p++){
		w = chartorune(&r, typing+b1);
		b1 += w;
		if(r=='\n' || r==0x04)
			ntypebreak--;
	}
	if(p != p1)
		error("deltype 2");
	memmove(typing+b0, typing+b1, ntypeb-b1);
	ntypeb -= b1-b0;
	ntyper -= p1-p0;
}

void
type(Event *e, int fd0, int afd, int dfd)
{
	int m, n, nr;
	byte buf[128];

	if(e->nr > 0)
		addtype(e->c1, e->q0-q.p, e->b, e->nb, e->nr);
	else{
		m = e->q0;
		while(m < e->q1){
			n = sprint(buf, "#%d", m);
			write(afd, buf, n);
			n = read(dfd, buf, sizeof buf);
			nr = nrunes(buf, n);
			while(m+nr > e->q1){
				do; while(n>0 && (buf[--n]&0xC0)==0x80);
				--nr;
			}
			if(n == 0)
				break;
			addtype(e->c1, m-q.p, buf, n, nr);
			m += nr;
		}
	}
	sendtype(fd0);
}

void
send(Event *e, int fd0, int cfd, int afd, int dfd, int donl)
{
	int l, m, n, nr, lastc, end;
	byte abuf[16], buf[128];

	end = q.p+ntyper;
	l = sprint(abuf, "#%d", end);
	write(afd, abuf, l);
	if(e->nr > 0){
		write(dfd, e->b, e->nb);
		addtype(e->c1, ntyper, e->b, e->nb, e->nr);
		lastc = e->r[e->nr-1];
	}else{
		m = e->q0;
		lastc = 0;
		while(m < e->q1){
			n = sprint(buf, "#%d", m);
			write(afd, buf, n);
			n = read(dfd, buf, sizeof buf);
			nr = nrunes(buf, n);
			while(m+nr > e->q1){
				do; while(n>0 && (buf[--n]&0xC0)==0x80);
				--nr;
			}
			if(n == 0)
				break;
			l = sprint(abuf, "#%d", end);
			write(afd, abuf, l);
			write(dfd, buf, n);
			addtype(e->c1, ntyper, buf, n, nr);
			lastc = buf[n-1];
			m += nr;
			end += nr;
		}
	}
	if(donl && lastc!='\n'){
		write(dfd, "\n", 1);
		addtype(e->c1, ntyper, "\n", 1, 1);
	}
	write(cfd, "dot=addr", 8);
	sendtype(fd0);
}
