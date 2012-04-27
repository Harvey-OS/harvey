/* password.c */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mp.h>
#include <libsec.h>
#include "SConn.h"
#include "secstore.h"

static Biobuf*
openPW(char *id, int mode)
{
	int nfn = strlen(SECSTORE_DIR)+strlen(id)+20;
	char *fn;
	Biobuf *b;

	if(validatefile(id) == nil || strcmp(id,".") == 0)
		return nil;
	fn = emalloc(nfn);
	snprint(fn, nfn, "%s/who/%s", SECSTORE_DIR, id);
	b = Bopen(fn, mode);
	free(fn);
	return b;
}

static ulong
mtimePW(char *id)
{
	ulong mt;
	char *fn;
	Dir *d;

	fn = smprint("%s/who/%s", SECSTORE_DIR, id);
	d = dirstat(fn);
	mt = (d? d->mtime: 0);
	free(d);
	free(fn);
	return mt;
}

PW *
getPW(char *id, int dead_or_alive)
{
	ulong now = time(0);
	char *f1, *f2, *oid;		/* fields 1, 2 = attribute, value */
	Biobuf *bin;
	PW *pw;

	oid = id;
	if((bin = openPW(id, OREAD)) == 0){
		id = "FICTITIOUS";
		if((bin = openPW(id, OREAD)) == 0){
			werrstr("accounts %s and FICTITIOUS do not exist", oid);
			return nil;
		}
	}
	pw = emalloc(sizeof *pw);
	pw->id = estrdup(id);
	pw->status |= Enabled;
	while( (f1 = Brdline(bin, '\n')) != 0){
		f1[Blinelen(bin)-1] = 0;
		for(f2 = f1; *f2 && *f2 != ' ' && *f2 != '\t'; f2++)
			;
		if(*f2)
			for(*f2++ = 0; *f2 && (*f2==' ' || *f2=='\t'); f2++)
				;
		if(strcmp(f1, "exp") == 0)
			pw->expire = strtoul(f2, 0, 10);
		else if(strcmp(f1, "DISABLED") == 0)
			pw->status &= ~Enabled;
		else if(strcmp(f1, "STA") == 0)
			pw->status |= STA;
		else if(strcmp(f1, "failed") == 0)
			pw->failed = strtoul(f2, 0, 10);
		else if(strcmp(f1, "other") == 0)
			pw->other = estrdup(f2);
		else if(strcmp(f1, "PAK-Hi") == 0)
			pw->Hi = strtomp(f2, nil, 64, nil);
	}
	Bterm(bin);
	if(pw->Hi == nil){
		werrstr("corrupted account file for %s", pw->id);
		freePW(pw);
		return nil;
	}
	if(dead_or_alive)
		return pw;  /* return for editing, whether valid now or not */
	if(pw->expire != 0 && pw->expire <= now){
		/* %.28s excludes ctime's newline */
		werrstr("account %s expired at %.28s", pw->id,
			ctime(pw->expire));
		freePW(pw);
		return nil;
	}
	if((pw->status & Enabled) == 0){
		werrstr("account %s disabled", pw->id);
		freePW(pw);
		return nil;
	}
	if(pw->failed < 10)
		return pw;	/* success */
	if(now < mtimePW(id)+300){
		werrstr("too many failures; try again in five minutes");
		freePW(pw);
		return nil;
	}
	pw->failed = 0;
	putPW(pw);	/* reset failed-login-counter after five minutes */
	return pw;
}

int
putPW(PW *pw)
{
	Biobuf *bout;
	char *hexHi;

	if((bout = openPW(pw->id, OWRITE|OTRUNC)) ==0){
		werrstr("can't open PW file for %s", pw->id);
		return -1;
	}
	Bprint(bout, "exp	%lud\n", pw->expire);
	if(!(pw->status & Enabled))
		Bprint(bout, "DISABLED\n");
	if(pw->status & STA)
		Bprint(bout, "STA\n");
	if(pw->failed)
		Bprint(bout, "failed\t%d\n", pw->failed);
	if(pw->other)
		Bprint(bout,"other\t%s\n", pw->other);
	hexHi = mptoa(pw->Hi, 64, nil, 0);
	Bprint(bout, "PAK-Hi\t%s\n", hexHi);
	free(hexHi);
	return 0;
}

void
freePW(PW *pw)
{
	if(pw == nil)
		return;
	free(pw->id);
	free(pw->other);
	mpfree(pw->Hi);
	free(pw);
}
