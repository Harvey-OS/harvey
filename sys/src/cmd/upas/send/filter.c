#include "common.h"
#include "send.h"

Biobuf	bin;

void
main(int argc, char *argv[])
{
	message *mp;
	dest *dp;
	Reprog *p;
	Resub match[10];
	char file[MAXPATHLEN];
	Biobuf *fp;
	char *rcvr, *cp;
	Lock *l;
	String *tmp;
	int i;
	int fromonly = 1;

	ARGBEGIN {
	case 'b':
		fromonly = 0;
		break;
	} ARGEND

	Binit(&bin, 0, OREAD);
	if(argc < 2){
		fprint(2, "usage: filter rcvr mailfile [regexp mailfile ...]\n");
		exits("usage");
	}
	mp = m_read(&bin, 1);

	/* get rid of local system name */
	cp = strchr(s_to_c(mp->sender), '!');
	if(cp){
		cp++;
		mp->sender = s_copy(cp);
	}

	dp = d_new(0);
	strcpy(file, argv[1]);
	for(i = 2; i < argc-1; i += 2){
		p = regcomp(argv[i]);
		if(p == 0)
			continue;
		if(regexec(p, s_to_c(mp->sender), match, 10)){
			regsub(argv[i+1], file, match, 10);
			break;
		}
		if(fromonly)
			continue;
		if(regexec(p, s_to_c(mp->body), match, 10)){
			regsub(argv[i+1], file, match, 10);
			break;
		}
	}

	/*
	 *  always lock the normal mail file to avoid too many lock files
	 *  lying about.  This isn't right but it's what the majority prefers.
	 */
	l = lock(argv[1]);
	if(l == 0){
		fprint(2, "can't lock mail file %s\n", argv[1]);
		exit(1);
	}

	/*
	 *  open the destination mail file
	 */
	fp = sysopen(file, "cal", MBOXMODE);
	if (fp == 0){
		tmp = s_append(0, file);
		s_append(tmp, ".tmp");
		fp = sysopen(s_to_c(tmp), "cal", MBOXMODE);
		if(fp == 0){
			unlock(l);
			fprint(2, "can't open mail file %s\n", file);
			exit(1);
		}
		syslog(0, "mail", "error: used %s", s_to_c(tmp));
		s_free(tmp);
	}
	if(m_print(mp, fp, (char *)0, 1) < 0
	|| Bprint(fp, "\n") < 0
	|| Bflush(fp) < 0){
		sysclose(fp);
		unlock(l);
		fprint(2, "can't write mail file %s\n", file);
		exit(1);
	}
	sysclose(fp);

	unlock(l);
	rcvr = argv[0];
	if(cp = strrchr(rcvr, '!'))
		rcvr = cp+1;
	logdelivery(dp, rcvr, mp);
	exit(0);
}
