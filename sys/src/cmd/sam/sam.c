#include "sam.h"

Rune	genbuf[BLOCKSIZE];
int	io;
int	panicking;
int	rescuing;
Mod	modnum;
String	genstr;
String	rhs;
String	wd;
String	cmdstr;
Rune	empty[] = { 0 };
char	*genc;
File	*curfile;
File	*flist;
File	*cmd;
jmp_buf	mainloop;
List tempfile;
int	quitok = TRUE;
int	downloaded;
int	dflag;
int	Rflag;
char	*machine;
char	*home;
int	bpipeok;
int	termlocked;
char	*samterm = SAMTERM;
char	*rsamname = RSAM;

Rune	baddir[] = { '<', 'b', 'a', 'd', 'd', 'i', 'r', '>', '\n'};

void	usage(void);

void
main(int argc, char *argv[])
{
	int i;
	String *t;
	char **ap, **arg;

	arg = argv++;
	ap = argv;
	while(argc>1 && argv[0] && argv[0][0]=='-'){
		switch(argv[0][1]){
		case 'd':
			dflag++;
			break;

		case 'r':
			--argc, argv++;
			if(argc == 1)
				usage();
			machine = *argv;
			break;

		case 'R':
			Rflag++;
			break;

		case 't':
			--argc, argv++;
			if(argc == 1)
				usage();
			samterm = *argv;
			break;

		case 's':
			--argc, argv++;
			if(argc == 1)
				usage();
			rsamname = *argv;
			break;

		default:
			dprint("sam: unknown flag %c\n", argv[0][1]);
			exits("usage");
		}
		--argc, argv++;
	}
	Strinit(&cmdstr);
	Strinit0(&lastpat);
	Strinit0(&lastregexp);
	Strinit0(&genstr);
	Strinit0(&rhs);
	Strinit0(&wd);
	tempfile.listptr = emalloc(0);
	Strinit0(&plan9cmd);
	home = getenv(HOME);
	if(home == 0)
		home = "/";
	if(!dflag)
		startup(machine, Rflag, arg, ap);
	Fstart();
	notify(notifyf);
	if(argc>1){
		for(i=0; i<argc-1; i++)
			if(!setjmp(mainloop)){
				t = tmpcstr(argv[i]);
				Straddc(t, '\0');
				Strduplstr(&genstr, t);
				freetmpstr(t);
				Fsetname(newfile(), &genstr);
			}
	}else if(!downloaded)
		newfile()->state = Clean;
	modnum++;
	if(file.nused)
		current(file.filepptr[0]);
	setjmp(mainloop);
	cmdloop();
	trytoquit();	/* if we already q'ed, quitok will be TRUE */
	exits(0);
}

void
usage(void)
{
	dprint("usage: sam [-d] [-t samterm] [-s sam name] -r machine\n");
	exits("usage");
}

void
rescue(void)
{
	int i, nblank = 0;
	File *f;
	char *c;
	char buf[256];

	if(rescuing++)
		return;
	io = -1;
	for(i=0; i<file.nused; i++){
		f = file.filepptr[i];
		if(f==cmd || f->nrunes==0 || f->state!=Dirty)
			continue;
		if(io == -1){
			sprint(buf, "%s/sam.save", home);
			io = create(buf, 1, 0777);
			if(io<0)
				return;
		}
		if(f->name.s[0]){
			c = Strtoc(&f->name);
			strncpy(buf, c, sizeof buf-1);
			buf[sizeof buf-1] = 0;
			free(c);
		}else
			sprint(buf, "nameless.%d", nblank++);
		fprint(io, "#!%s '%s' $* <<'---%s'\n", SAMSAVECMD, buf, buf);
		addr.r.p1 = 0, addr.r.p2 = f->nrunes;
		writeio(f);
		fprint(io, "\n---%s\n", (char *)buf);
	}
}

void
panic(char *s)
{
	int wasd;

	if(!panicking++ && !setjmp(mainloop)){
		wasd = downloaded;
		downloaded = 0;
		dprint("sam: panic: %s: %r\n", s);
		if(wasd)
			fprint(2, "sam: panic: %s: %r\n", s);
		rescue();
		abort();
	}
}

void
hiccough(char *s)
{
	if(rescuing)
		exits("rescue");
	if(s)
		dprint("%s\n", s);
	resetcmd();
	resetxec();
	resetsys();
	if(io > 0)
		close(io);
	if(undobuf->nrunes)
		Bdelete(undobuf, (Posn)0, undobuf->nrunes);
	update();
	if (curfile) {
		if (curfile->state==Unread)
			curfile->state = Clean;
		else if (downloaded)
			outTs(Hcurrent, curfile->tag);
	}
	longjmp(mainloop, 1);
}

void
intr(void)
{
	error(Eintr);
}

void
trytoclose(File *f)
{
	char *t;
	char buf[256];

	if(f == cmd)	/* possible? */
		return;
	if(f->deleted)
		return;
	if(f->state==Dirty && !f->closeok){
		f->closeok = TRUE;
		if(f->name.s[0]){
			t = Strtoc(&f->name);
			strncpy(buf, t, sizeof buf-1);
			free(t);
		}else
			strcpy(buf, "nameless file");
		error_s(Emodified, buf);
	}
	f->deleted = TRUE;
}

void
trytoquit(void)
{
	int c;
	File *f;

	if(!quitok)
{
		for(c = 0; c<file.nused; c++){
			f = file.filepptr[c];
			if(f!=cmd && f->state==Dirty){
				quitok = TRUE;
				eof = FALSE;
				error(Echanges);
			}
		}
}
}

void
load(File *f)
{
	Address saveaddr;

	Strduplstr(&genstr, &f->name);
	filename(f);
	if(f->name.s[0]){
		saveaddr = addr;
		edit(f, 'I');
		addr = saveaddr;
	}else
		f->state = Clean;
	Fupdate(f, TRUE, TRUE);
}

void
cmdupdate(void)
{
	if(cmd && cmd->mod!=0){
		Fupdate(cmd, FALSE, downloaded);
		cmd->dot.r.p1 = cmd->dot.r.p2 = cmd->nrunes;
		telldot(cmd);
	}
}

void
delete(File *f)
{
	if(downloaded && f->rasp)
		outTs(Hclose, f->tag);
	delfile(f);
	if(f == curfile)
		current(0);
}

void
update(void)
{
	int i, anymod;
	File *f;

	settempfile();
	for(anymod = i=0; i<tempfile.nused; i++){
		f = tempfile.filepptr[i];
		if(f==cmd)	/* cmd gets done in main() */
			continue;
		if(f->deleted)
			delete(f);
		if(f->mod==modnum && Fupdate(f, FALSE, downloaded))
			anymod++;
		if(f->rasp)
			telldot(f);
	}
	if(anymod)
		modnum++;
}

File *
current(File *f)
{
	return curfile = f;
}

void
edit(File *f, int cmd)
{
	int empty = TRUE;
	Posn p;
	int nulls;

	if(cmd == 'r')
		Fdelete(f, addr.r.p1, addr.r.p2);
	if(cmd=='e' || cmd=='I'){
		Fdelete(f, (Posn)0, f->nrunes);
		addr.r.p2 = f->nrunes;
	}else if(f->nrunes!=0 || (f->name.s[0] && Strcmp(&genstr, &f->name)!=0))
		empty = FALSE;
	if((io = open(genc, OREAD))<0) {
		if (curfile && curfile->state == Unread)
			curfile->state = Clean;
		error_s(Eopen, genc);
	}
	p = readio(f, &nulls, empty);
	closeio((cmd=='e' || cmd=='I')? -1 : p);
	if(cmd == 'r')
		f->ndot.r.p1 = addr.r.p2, f->ndot.r.p2 = addr.r.p2+p;
	else
		f->ndot.r.p1 = f->ndot.r.p2 = 0;
	f->closeok = empty;
	if (quitok)
		quitok = empty;
	else
		quitok = FALSE;
	state(f, empty && !nulls? Clean : Dirty);
	if(cmd == 'e')
		filename(f);
}

int
getname(File *f, String *s, int save)
{
	int c, i;

	Strzero(&genstr);
	if(genc){
		free(genc);
		genc = 0;
	}
	if(s==0 || (c = s->s[0])==0){		/* no name provided */
		if(f)
			Strduplstr(&genstr, &f->name);
		else
			Straddc(&genstr, '\0');
		goto Return;
	}
	if(c!=' ' && c!='\t')
		error(Eblank);
	for(i=0; (c=s->s[i])==' ' || c=='\t'; i++)
		;
	while(s->s[i] > ' ')
		Straddc(&genstr, s->s[i++]);
	if(s->s[i])
		error(Enewline);
	Straddc(&genstr, '\0');
	if(f && (save || f->name.s[0]==0)){
		Fsetname(f, &genstr);
		if(Strcmp(&f->name, &genstr)){
			quitok = f->closeok = FALSE;
			f->qid = 0;
			f->date = 0;
			state(f, Dirty); /* if it's 'e', fix later */
		}
	}
    Return:
	genc = Strtoc(&genstr);
	return genstr.n-1;	/* strlen(name) */
}

void
filename(File *f)
{
	if(genc)
		free(genc);
	genc = Strtoc(&genstr);
	dprint("%c%c%c %s\n", " '"[f->state==Dirty],
		"-+"[f->rasp!=0], " ."[f==curfile], genc);
}

void
undostep(File *f)
{
	Buffer *t;
	int changes;
	Mark mark;

	t = f->transcript;
	changes = Fupdate(f, TRUE, TRUE);
	Bread(t, (Rune*)&mark, (sizeof mark)/RUNESIZE, f->markp);
	Bdelete(t, f->markp, t->nrunes);
	f->markp = mark.p;
	f->dot.r = mark.dot;
	f->ndot.r = mark.dot;
	f->mark = mark.mark;
	f->mod = mark.m;
	f->closeok = mark.s1!=Dirty;
	if(mark.s1==Dirty)
		quitok = FALSE;
	if(f->state==Clean && mark.s1==Clean && changes)
		state(f, Dirty);
	else
		state(f, mark.s1);
}

int
undo(void)
{
	File *f;
	int i;
	Mod max;
	if((max = curfile->mod)==0)
		return 0;
	settempfile();
	for(i = 0; i<tempfile.nused; i++){
		f = tempfile.filepptr[i];
		if(f!=cmd && f->mod==max)
			undostep(f);
	}
	return 1;
}

int
readcmd(String *s)
{
	int retcode;

	if(flist == 0)
		(flist = Fopen())->state = Clean;
	addr.r.p1 = 0, addr.r.p2 = flist->nrunes;
	retcode = plan9(flist, '<', s, FALSE);
	Fupdate(flist, FALSE, FALSE);
	flist->mod = 0;
	if (flist->nrunes > BLOCKSIZE)
		error(Etoolong);
	Strzero(&genstr);
	Strinsure(&genstr, flist->nrunes);
	Fchars(flist, genbuf, (Posn)0, flist->nrunes);
	memmove(genstr.s, genbuf, flist->nrunes*RUNESIZE);
	genstr.n = flist->nrunes;
	Straddc(&genstr, '\0');
	return retcode;
}

void
cd(String *str)
{
	int i;
	File *f;
	String *t;

	t = tmpcstr("/bin/pwd");
	Straddc(t, '\0');
	if (flist) {
		Fclose(flist);
		flist = 0;
	}
	if (readcmd(t) != 0) {
		Strduplstr(&genstr, tmprstr(baddir, sizeof(baddir)/sizeof(Rune)));
		Straddc(&genstr, '\0');
	}
	freetmpstr(t);
	Strduplstr(&wd, &genstr);
	if(wd.s[0] == 0){
		wd.n = 0;
		warn(Wpwd);
	}else if(wd.s[wd.n-2] == '\n'){
		--wd.n;
		wd.s[wd.n-1]='/';
	}
	if(chdir(getname((File *)0, str, FALSE)? genc : home))
		syserror("chdir");
	settempfile();
	for(i=0; i<tempfile.nused; i++){
		f = tempfile.filepptr[i];
		if(f!=cmd && f->name.s[0]!='/' && f->name.s[0]!=0){
			Strinsert(&f->name, &wd, (Posn)0);
			sortname(f);
		}
	}
}

int
loadflist(String *s)
{
	int c, i;

	c = s->s[0];
	for(i = 0; s->s[i]==' ' || s->s[i]=='\t'; i++)
		;
	if((c==' ' || c=='\t') && s->s[i]!='\n'){
		if(s->s[i]=='<'){
			Strdelete(s, 0L, (long)i+1);
			readcmd(s);
		}else{
			Strzero(&genstr);
			while((c = s->s[i++]) && c!='\n')
				Straddc(&genstr, c);
			Straddc(&genstr, '\0');
		}
	}else{
		if(c != '\n')
			error(Eblank);
		Strdupl(&genstr, empty);
	}
	if(genc)
		free(genc);
	genc = Strtoc(&genstr);
	return genstr.s[0];
}

File *
readflist(int readall, int delete)
{
	Posn i;
	int c;
	File *f;
	String *t;

	for(i=0,f=0; f==0 || readall || delete; i++){	/* ++ skips blank */
		Strdelete(&genstr, (Posn)0, i);
		for(i=0; (c = genstr.s[i])==' ' || c=='\t' || c=='\n'; i++)
			;
		if(i >= genstr.n)
			break;
		Strdelete(&genstr, (Posn)0, i);
		for(i=0; (c=genstr.s[i]) && c!=' ' && c!='\t' && c!='\n'; i++)
			;

		if(i == 0)
			break;
		genstr.s[i] = 0;
		t = tmprstr(genstr.s, i+1);
		f = lookfile(t);
		if(delete){
			if(f == 0)
				warn_S(Wfile, t);
			else
				trytoclose(f);
		}else if(f==0 && readall)
			Fsetname(f = newfile(), t);
	}
	return f;
}

File *
tofile(String *s)
{
	File *f;

	if(s->s[0] != ' ')
		error(Eblank);
	if(loadflist(s) == 0){
		f = lookfile(&genstr);	/* empty string ==> nameless file */
		if(f == 0)
			error_s(Emenu, genc);
	}else if((f=readflist(FALSE, FALSE)) == 0)
		error_s(Emenu, genc);
	return current(f);
}

File *
getfile(String *s)
{
	File *f;

	if(loadflist(s) == 0)
		Fsetname(f = newfile(), &genstr);
	else if((f=readflist(TRUE, FALSE)) == 0)
		error(Eblank);
	return current(f);
}

void
closefiles(File *f, String *s)
{
	if(s->s[0] == 0){
		if(f == 0)
			error(Enofile);
		trytoclose(f);
		return;
	}
	if(s->s[0] != ' ')
		error(Eblank);
	if(loadflist(s) == 0)
		error(Enewline);
	readflist(FALSE, TRUE);
}

void
copy(File *f, Address addr2)
{
	Posn p;
	int ni;
	for(p=addr.r.p1; p<addr.r.p2; p+=ni){
		ni = addr.r.p2-p;
		if(ni > BLOCKSIZE)
			ni = BLOCKSIZE;
		Fchars(f, genbuf, p, p+ni);
		Finsert(addr2.f, tmprstr(genbuf, ni), addr2.r.p2);
	}
	addr2.f->ndot.r.p2 = addr2.r.p2+(f->dot.r.p2-f->dot.r.p1);
	addr2.f->ndot.r.p1 = addr2.r.p2;
}

void
move(File *f, Address addr2)
{
	if(addr.r.p2 <= addr2.r.p2){
		Fdelete(f, addr.r.p1, addr.r.p2);
		copy(f, addr2);
	}else if(addr.r.p1 >= addr2.r.p2){
		copy(f, addr2);
		Fdelete(f, addr.r.p1, addr.r.p2);
	}else
		error(Eoverlap);
}

Posn
nlcount(File *f, Posn p0, Posn p1)
{
	Posn nl = 0;

	Fgetcset(f, p0);
	while(p0++<p1)
		if(Fgetc(f)=='\n')
			nl++;
	return nl;
}

void
printposn(File *f, int charsonly)
{
	Posn l1, l2;

	if(!charsonly){
		l1 = 1+nlcount(f, (Posn)0, addr.r.p1);
		l2 = l1+nlcount(f, addr.r.p1, addr.r.p2);
		/* check if addr ends with '\n' */
		if(addr.r.p2>0 && addr.r.p2>addr.r.p1 && (Fgetcset(f, addr.r.p2-1),Fgetc(f)=='\n'))
			--l2;
		dprint("%lud", l1);
		if(l2 != l1)
			dprint(",%lud", l2);
		dprint("; ");
	}
	dprint("#%lud", addr.r.p1);
	if(addr.r.p2 != addr.r.p1)
		dprint(",#%lud", addr.r.p2);
	dprint("\n");
}

void
settempfile(void)
{
	if(tempfile.nalloc < file.nused){
		free(tempfile.listptr);
		tempfile.listptr = emalloc(sizeof(*tempfile.filepptr)*file.nused);
		tempfile.nalloc = file.nused;
	}
	tempfile.nused = file.nused;
	memmove(&tempfile.filepptr[0], &file.filepptr[0], file.nused*sizeof(File*));
}
