/*
 * Count bytes within runes, if it fits in a uvlong, and other things.
 */
#include <u.h>
#include <libc.h>
#include <bio.h>

/* flags, per-file counts, and total counts */
static int pline, pword, prune, pbadr, pchar;
static uvlong nline, nword, nrune, nbadr, nchar;
static uvlong tnline, tnword, tnrune, tnbadr, tnchar;

enum{Space, Word};

static void
wc(Biobuf *bin)
{
	int where;
	long r;

	nline = 0;
	nword = 0;
	nrune = 0;
	nbadr = 0;
	where = Space;
	while ((long)(r = Bgetrune(bin)) >= 0) {
		nrune++;
		if(r == Runeerror) {
			nbadr++;
			continue;
		}
		if(r == '\n')
			nline++;
		if(where == Word){
			if(isspacerune(r))
				where = Space;
		}else
			if(isspacerune(r) == 0){
				where = Word;
				nword++;
			}
	}
	nchar = Boffset(bin);
	tnline += nline;
	tnword += nword;
	tnrune += nrune;
	tnbadr += nbadr;
	tnchar += nchar;
}

static void
report(uvlong nline, uvlong nword, uvlong nrune, uvlong nbadr, uvlong nchar, char *fname)
{
	char line[1024], *s, *e;

	s = line;
	e = line + sizeof line;
	line[0] = 0;
	if(pline)
		s = seprint(s, e, " %7llud", nline);
	if(pword)
		s = seprint(s, e, " %7llud", nword);
	if(prune)
		s = seprint(s, e, " %7llud", nrune);
	if(pbadr)
		s = seprint(s, e, " %7llud", nbadr);
	if(pchar)
		s = seprint(s, e, " %7llud", nchar);
	if(fname != nil)
		seprint(s, e, " %s",   fname);
	print("%s\n", line+1);
}

void
main(int argc, char *argv[])
{
	char *sts;
	Biobuf sin, *bin;
	int i;

	sts = nil;
	ARGBEGIN {
	case 'l': pline++; break;
	case 'w': pword++; break;
	case 'r': prune++; break;
	case 'b': pbadr++; break;
	case 'c': pchar++; break;
	default:
		fprint(2, "Usage: %s [-lwrbc] [file ...]\n", argv0);
		exits("usage");
	} ARGEND
	if(pline+pword+prune+pbadr+pchar == 0){
		pline = 1;
		pword = 1;
		pchar = 1;
	}
	if(argc == 0){
		Binit(&sin, 0, OREAD);
		wc(&sin);
		report(nline, nword, nrune, nbadr, nchar, nil);
		Bterm(&sin);
	}else{
		for(i = 0; i < argc; i++){
			bin = Bopen(argv[i], OREAD);
			if(bin == nil){
				perror(argv[i]);
				sts = "can't open";
				continue;
			}
			wc(bin);
			report(nline, nword, nrune, nbadr, nchar, argv[i]);
			Bterm(bin);
		}
		if(argc>1)
			report(tnline, tnword, tnrune, tnbadr, tnchar, "total");
	}
	exits(sts);
}
