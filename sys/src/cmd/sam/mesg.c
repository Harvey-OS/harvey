#include "sam.h"

Header	h;
uchar	indata[DATASIZE];
uchar	outdata[2*DATASIZE+3];	/* room for overflow message */
uchar	*inp;
uchar	*outp;
uchar	*outmsg = outdata;
Posn	cmdpt;
Posn	cmdptadv;
Buffer	*snarfbuf;
int	waitack;
int	noflush;
int	tversion;

long	inlong(void);
int	inshort(void);
int	inmesg(Tmesg);
void	setgenstr(File*, Posn, Posn);

#ifdef DEBUG
char *hname[] = {
	[Hversion]	"Hversion",
	[Hbindname]	"Hbindname",
	[Hcurrent]	"Hcurrent",
	[Hnewname]	"Hnewname",
	[Hmovname]	"Hmovname",
	[Hgrow]		"Hgrow",
	[Hcheck0]	"Hcheck0",
	[Hcheck]	"Hcheck",
	[Hunlock]	"Hunlock",
	[Hdata]		"Hdata",
	[Horigin]	"Horigin",
	[Hunlockfile]	"Hunlockfile",
	[Hsetdot]	"Hsetdot",
	[Hgrowdata]	"Hgrowdata",
	[Hmoveto]	"Hmoveto",
	[Hclean]	"Hclean",
	[Hdirty]	"Hdirty",
	[Hcut]		"Hcut",
	[Hsetpat]	"Hsetpat",
	[Hdelname]	"Hdelname",
	[Hclose]	"Hclose",
	[Hsetsnarf]	"Hsetsnarf",
	[Hsnarflen]	"Hsnarflen",
	[Hack]		"Hack",
	[Hexit]		"Hexit",
};

char *tname[] = {
	[Tversion]	"Tversion",
	[Tstartcmdfile]	"Tstartcmdfile",
	[Tcheck]	"Tcheck",
	[Trequest]	"Trequest",
	[Torigin]	"Torigin",
	[Tstartfile]	"Tstartfile",
	[Tworkfile]	"Tworkfile",
	[Ttype]		"Ttype",
	[Tcut]		"Tcut",
	[Tpaste]	"Tpaste",
	[Tsnarf]	"Tsnarf",
	[Tstartnewfile]	"Tstartnewfile",
	[Twrite]	"Twrite",
	[Tclose]	"Tclose",
	[Tsearch]	"Tsearch",
	[Tlook]		"Tlook",
	[Tsend]		"Tsend",
	[Tstartsnarf]	"Tstartsnarf",
	[Tsetsnarf]	"Tsetsnarf",
	[Tdclick]	"Tdclick",
	[Tack]		"Tack",
	[Texit]		"Texit",
};

void
journal(int out, char *s)
{
	static fd = 0;

	if(fd <= 0)
		fd = create("/tmp/sam.out", 1, 0666L);
	fprint(fd, "%s%s\n", out? "out: " : "in:  ", s);
}

void
journaln(int out, int n)
{
	char buf[32];
	sprint(buf, "%d", n);
	journal(out, buf);
}
#else
#define	journal(a, b)
#define journaln(a, b)
#endif

int
rcvchar(void){
	static uchar buf[64];
	static i, nleft = 0;

	if(nleft <= 0){
		nleft = read(0, (char *)buf, sizeof buf);
		if(nleft <= 0)
			return -1;
		i = 0;
	}
	--nleft;
	return buf[i++];
}

int
rcv(void){
	int c;
	static state = 0;
	static count = 0;
	static i = 0;

	while((c=rcvchar()) != -1)
		switch(state){
		case 0:
			h.type = c;
			state++;
			break;

		case 1:
			h.count0 = c;
			state++;
			break;

		case 2:
			h.count1 = c;
			count = h.count0|(h.count1<<8);
			i = 0;
			if(count > DATASIZE)
				panic("count>DATASIZE");
			if(count == 0)
				goto zerocount;
			state++;
			break;

		case 3:
			indata[i++] = c;
			if(i == count){
		zerocount:
				indata[i] = 0;
				state = count = 0;
				return inmesg(h.type);
			}
			break;
		}
	return 0;
}

File *
whichfile(int tag)
{
	int i;

	for(i = 0; i<file.nused; i++)
		if(file.filepptr[i]->tag==tag)
			return file.filepptr[i];
	hiccough((char *)0);
	return 0;
}

int
inmesg(Tmesg type)
{
	Rune buf[1025];
	int i, m;
	short s;
	long l, l1;
	File *f;
	Posn p0, p1;
	Range r;
	String *str;
	char *c;

	if(type > TMAX)
		panic("inmesg");

	journal(0, tname[type]);

	inp = indata;
	SET(c);
	switch(type){
	case -1:
		panic("rcv error");

	default:
		fprint(2, "unknown type %d\n", type);
		panic("rcv unknown");

	case Tversion:
		tversion = inshort();
		journaln(0, tversion);
		break;

	case Tstartcmdfile:
		l = inlong();
		journaln(0, l);
		Strdupl(&genstr, samname);
		cmd = newfile();
		outTsl(Hbindname, cmd->tag, l);
		outTs(Hcurrent, cmd->tag);
		Fsetname(cmd, &genstr);
		cmd->rasp = emalloc(sizeof(List));
		cmd->state = Clean;
		if(cmdstr.n){
			Finsert(cmd, &cmdstr, 0L);
			Strdelete(&cmdstr, 0L, (Posn)cmdstr.n);
		}
		Fupdate(cmd, FALSE, TRUE);
		outT0(Hunlock);
		break;

	case Tcheck:
		/* go through whichfile to check the tag */
		outTs(Hcheck, whichfile(inshort())->tag);
		break;

	case Trequest:
		f = whichfile(inshort());
		p0 = inlong();
		p1 = p0+inshort();
		journaln(0, p0);
		journaln(0, p1-p0);
		if(f->state == Unread)
			panic("Trequest: unread");
		if(p1>f->nrunes)
			p1 = f->nrunes;
		if(p0>f->nrunes) /* can happen e.g. scrolling during command */
			p0 = f->nrunes;
		if(p0 == p1){
			i = 0;
			r.p1 = r.p2 = p0;
		}else{
			r = rdata(f->rasp, p0, p1-p0);
			i = r.p2-r.p1;
			if(Fchars(f, buf, r.p1, r.p2)!=i)
				panic("Trequest 2");
		}
		buf[i]=0;
		outTslS(Hdata, f->tag, r.p1, tmprstr(buf, i+1));
		break;

	case Torigin:
		s = inshort();
		l = inlong();
		l1 = inlong();
		journaln(0, l1);
		lookorigin(whichfile(s), l, l1);
		break;

	case Tstartfile:
		f = whichfile(inshort());
		if(!f->rasp)	/* this might be a duplicate message */
			f->rasp = emalloc(sizeof(List));
		current(f);
		outTsl(Hbindname, f->tag, inlong());
		outTs(Hcurrent, f->tag);
		journaln(0, f->tag);
		termlocked++;
		if(f->state == Unread)
			load(f);
		else{
			if(f->nrunes>0){
				rgrow(f->rasp, 0L, f->nrunes);
				outTsll(Hgrow, f->tag, 0L, f->nrunes);
			}
			outTs(Hcheck0, f->tag);
			moveto(f, f->dot.r);
		}
		break;

	case Tworkfile:
		i = inshort();
		f = whichfile(i);
		current(f);
		f->dot.r.p1 = inlong();
		f->dot.r.p2 = inlong();
		f->tdot = f->dot.r;
		journaln(0, i);
		journaln(0, f->dot.r.p1);
		journaln(0, f->dot.r.p2);
		break;

	case Ttype:
		f = whichfile(inshort());
		p0 = inlong();
		journaln(0, p0);
		journal(0, (char*)inp);
		str = tmpcstr((char*)inp);
		i = str->n;
		Finsert(f, str, p0);
		if(Fupdate(f, FALSE, FALSE))
			modnum++;
		if(f==cmd && p0==f->nrunes-i && i>0 && str->s[i-1]=='\n'){
			freetmpstr(str);
			termlocked++;
			termcommand();
		}else
			freetmpstr(str);
		f->dot.r.p1 = f->dot.r.p2 = p0+i; /* terminal knows this already */
		f->tdot = f->dot.r;
		break;

	case Tcut:
		f = whichfile(inshort());
		p0 = inlong();
		p1 = inlong();
		journaln(0, p0);
		journaln(0, p1);
		Fdelete(f, p0, p1);
		if(Fupdate(f, FALSE, FALSE))
			modnum++;
		f->dot.r.p1 = f->dot.r.p2 = p0;
		f->tdot = f->dot.r;   /* terminal knows the value of dot already */
		break;

	case Tpaste:
		f = whichfile(inshort());
		p0 = inlong();
		journaln(0, p0);
		for(l=0; l<snarfbuf->nrunes; l+=m){
			m = snarfbuf->nrunes-l;
			if(m>BLOCKSIZE)
				m = BLOCKSIZE;
			Bread(snarfbuf, genbuf, m, l);
			Finsert(f, tmprstr(genbuf, m), p0);
		}
		if(Fupdate(f, FALSE, TRUE))
			modnum++;
		f->dot.r.p1 = p0;
		f->dot.r.p2 = p0+snarfbuf->nrunes;
		f->tdot.p1 = -1; /* force telldot to tell (arguably a BUG) */
		telldot(f);
		outTs(Hunlockfile, f->tag);
		break;

	case Tsnarf:
		i = inshort();
		p0 = inlong();
		p1 = inlong();
		snarf(whichfile(i), p0, p1, snarfbuf, 0);
		break;

	case Tstartnewfile:
		l = inlong();
		Strdupl(&genstr, empty);
		f = newfile();
		f->rasp = emalloc(sizeof(List));
		outTsl(Hbindname, f->tag, l);
		Fsetname(f, &genstr);
		outTs(Hcurrent, f->tag);
		current(f);
		load(f);
		break;

	case Twrite:
		termlocked++;
		i = inshort();
		journaln(0, i);
		f = whichfile(i);
		addr.r.p1 = 0;
		addr.r.p2 = f->nrunes;
		if(f->name.s[0] == 0)
			error(Enoname);
		Strduplstr(&genstr, &f->name);
		writef(f);
		break;

	case Tclose:
		i = inshort();
		journaln(0, i);
		f = whichfile(i);
		current(f);
		trytoclose(f);
		break;

	case Tlook:
		f = whichfile(inshort());
		termlocked++;
		p0 = inlong();
		p1 = inlong();
		journaln(0, p0);
		journaln(0, p1);
		setgenstr(f, p0, p1);
		for(l = 0; l<genstr.n; l++){
			i = genstr.s[l];
			if(utfrune(".*+?(|)\\[]^$", i))
				Strinsert(&genstr, tmpcstr("\\"), l++);
		}
		Straddc(&genstr, '\0');
		nextmatch(f, &genstr, p1, 1);
		moveto(f, sel.p[0]);
		break;

	case Tsearch:
		termlocked++;
		if(curfile == 0)
			error(Enofile);
		if(lastpat.s[0] == 0)
			panic("Tsearch");
		nextmatch(curfile, &lastpat, curfile->dot.r.p2, 1);
		moveto(curfile, sel.p[0]);
		break;

	case Tsend:
		termlocked++;
		inshort();	/* ignored */
		p0 = inlong();
		p1 = inlong();
		setgenstr(cmd, p0, p1);
		Bdelete(snarfbuf, (Posn)0, snarfbuf->nrunes);
		Binsert(snarfbuf, &genstr, (Posn)0);
		outTl(Hsnarflen, genstr.n);
		if(genstr.s[genstr.n-1] != '\n')
			Straddc(&genstr, '\n');
		Finsert(cmd, &genstr, cmd->nrunes);
		Fupdate(cmd, FALSE, TRUE);
		cmd->dot.r.p1 = cmd->dot.r.p2 = cmd->nrunes;
		telldot(cmd);
		termcommand();
		break;

	case Tdclick:
		f = whichfile(inshort());
		p1 = inlong();
		doubleclick(f, p1);
		f->tdot.p1 = f->tdot.p2 = p1;
		telldot(f);
		outTs(Hunlockfile, f->tag);
		break;

	case Tstartsnarf:
		i = -1;
		if(snarfbuf->nrunes < BLOCKSIZE){
			Bread(snarfbuf, genbuf, snarfbuf->nrunes, 0);
			c = Strtoc(tmprstr(genbuf, snarfbuf->nrunes));
			i = strlen(c);
			if(i > SNARFSIZE){
				free(c);
				i = -1;
			}
		}
		if(i < 0){
			dprint("snarf buffer too long\n");
			i = 0;
		}
		outTs(Hsetsnarf, i);
		if(i){
			Write(1, c, i);
			free(c);
		}
		break;

	case Tsetsnarf:
		m = inshort();
		if(m > SNARFSIZE)
			error(Etoolong);
		c = malloc(m+1);
		if(c){
			for(i=0; i<m; i++)
				c[i] = rcvchar();
			c[m] = 0;
			str = tmpcstr(c);
			free(c);
			Bdelete(snarfbuf, (Posn)0, snarfbuf->nrunes);
			Binsert(snarfbuf, str, (Posn)0);
			freetmpstr(str);
			outT0(Hunlock);
		}
		break;

	case Tack:
		waitack = 0;
		break;

	case Texit:
		exits(0);
	}
	return TRUE;
}

void
snarf(File *f, Posn p1, Posn p2, Buffer *buf, int emptyok)
{
	Posn l;
	int i;

	if(!emptyok && p1==p2)
		return;
	Bdelete(buf, (Posn)0, buf->nrunes);
	/* Stage through genbuf to avoid compaction problems (vestigial) */
	for(l=p1; l<p2; l+=i){
		i = p2-l>BLOCKSIZE? BLOCKSIZE : p2-l;
		Fchars(f, genbuf, l, l+i);
		Binsert(buf, tmprstr(genbuf, i), buf->nrunes);
	}
}

int
inshort(void)
{
	ushort n;

	n = inp[0] | (inp[1]<<8);
	inp += 2;
	return n;
}

long
inlong(void)
{
	ulong n;

	n = inp[0] | (inp[1]<<8) | (inp[2]<<16) | (inp[3]<<24);
	inp += 4;
	return n;
}

void
setgenstr(File *f, Posn p0, Posn p1)
{
	if(p0 != p1){
		if(p1-p0 >= TBLOCKSIZE)
			error(Etoolong);
		Strinsure(&genstr, p1-p0);
		Fchars(f, genbuf, p0, p1);
		memmove(genstr.s, genbuf, RUNESIZE*(p1-p0));
		genstr.n = p1-p0;
	}else{
		if(snarfbuf->nrunes == 0)
			error(Eempty);
		if(snarfbuf->nrunes > TBLOCKSIZE)
			error(Etoolong);
		Bread(snarfbuf, genbuf, snarfbuf->nrunes, (Posn)0);
		Strinsure(&genstr, snarfbuf->nrunes);
		memmove(genstr.s, genbuf, RUNESIZE*snarfbuf->nrunes);
		genstr.n = snarfbuf->nrunes;
	}
}

void
outT0(Hmesg type)
{
	outstart(type);
	outsend();
}

void
outTl(Hmesg type, long l)
{
	outstart(type);
	outlong(l);
	outsend();
}

void
outTs(Hmesg type, int s)
{
	outstart(type);
	journaln(1, s);
	outshort(s);
	outsend();
}

void
outS(String *s)
{
	char *c;
	int i;

	c = Strtoc(s);
	i = strlen(c);
	outcopy(i, c);
	if(i > 99)
		c[99] = 0;
	journaln(1, i);
	journal(1, c);
	free(c);
}

void
outTsS(Hmesg type, int s1, String *s)
{
	outstart(type);
	outshort(s1);
	outS(s);
	outsend();
}

void
outTslS(Hmesg type, int s1, Posn l1, String *s)
{
	outstart(type);
	outshort(s1);
	journaln(1, s1);
	outlong(l1);
	journaln(1, l1);
	outS(s);
	outsend();
}

void
outTS(Hmesg type, String *s)
{
	outstart(type);
	outS(s);
	outsend();
}

void
outTsllS(Hmesg type, int s1, Posn l1, Posn l2, String *s)
{
	outstart(type);
	outshort(s1);
	outlong(l1);
	outlong(l2);
	journaln(1, l1);
	journaln(1, l2);
	outS(s);
	outsend();
}

void
outTsll(Hmesg type, int s, Posn l1, Posn l2)
{
	outstart(type);
	outshort(s);
	outlong(l1);
	outlong(l2);
	journaln(1, l1);
	journaln(1, l2);
	outsend();
}

void
outTsl(Hmesg type, int s, Posn l)
{
	outstart(type);
	outshort(s);
	outlong(l);
	journaln(1, l);
	outsend();
}

void
outstart(Hmesg type)
{
	journal(1, hname[type]);
	outmsg[0] = type;
	outp = outmsg+3;
}

void
outcopy(int count, void *data)
{
	memmove(outp, data, count);
	outp += count;
}

void
outshort(int s)
{
	*outp++ = s;
	*outp++ = s>>8; 
}

void
outlong(long l)
{
	*outp++ = l;
	*outp++ = l>>8;
	*outp++ = l>>16;
	*outp++ = l>>24;
}

void
outsend(void)
{
	int outcount;

	outcount = outp-outmsg;
	outcount -= 3;
	outmsg[1] = outcount;
	outmsg[2] = outcount>>8;
	outmsg = outp;
	if(!noflush){
		outcount = outmsg-outdata;
		if (write(1, (char*) outdata, outcount) != outcount)
			rescue();
		outmsg = outdata;
		return;
	}
	if(outmsg < outdata+DATASIZE)
		return;
	outflush();
}

void
outflush(void)
{
	if(outmsg == outdata)
		return;
	noflush = 0;
	outT0(Hack);
	waitack = 1;
	do
		if(rcv() == 0){
			rescue();
			exits("eof");
		}
	while(waitack);
	outmsg = outdata;
	noflush = 1;
}
