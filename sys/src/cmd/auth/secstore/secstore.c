/* network login client */
#include <u.h>
#include <libc.h>
#include <mp.h>
#include <libsec.h>
#include "SConn.h"
#include "secstore.h"
enum{ CHK = 16, MAXFILES = 100 };

int verbose;

void
usage(void)
{
	fprint(2, "usage: secstore [-c] [-g getfile] [-p putfile] [-r rmfile] [-s tcp!server!5356] [-u user] [-v]\n");
	exits("usage");
}

static int
getfile(SConn *conn, char *gf, uchar **buf, ulong *buflen, uchar *key, int nkey)
{
	int fd = -1;
	int i, n, nr, nw, len;
	char s[Maxmsg+1];
	uchar skey[SHA1dlen], ib[Maxmsg+CHK], *ibr, *ibw, *bufw, *bufe;
	AESstate aes;
	DigestState *sha;

	if(strchr(gf, '/')){
		fprint(2, "simple filenames, not paths like %s\n", gf);
		return -1;
	}
	memset(&aes, 0, sizeof aes);

	snprint(s, Maxmsg, "GET %s\n", gf);
	conn->write(conn, (uchar*)s, strlen(s));

	/* get file size */
	s[0] = '\0';
	bufw = bufe = nil;
	if(readstr(conn, s) < 0){
		fprint(2, "remote: %s\n", s);
		return -1;
	}
	if((len = atoi(s)) < 0){
		fprint(2, "remote file %s does not exist\n", gf);
		return -1;
	}else if(len > MAXFILESIZE){//assert
		fprint(2, "implausible filesize %d for %s\n", len, gf);
		return -1;
	}
	if(buf != nil){
		*buflen = len - AESbsize - CHK;
		*buf = bufw = emalloc(len);
		bufe = bufw + len;
	}

	/* directory listing */
	if(strcmp(gf,".")==0){
		if(buf != nil)
			*buflen = len;
		for(i=0; i < len; i += n){
			if((n = conn->read(conn, (uchar*)s, Maxmsg)) <= 0){
				fprint(2, "empty file chunk\n");
				return -1;
			}
			if(buf == nil)
				write(1, s, n);
			else
				memmove((*buf)+i, s, n);
		}
		return 0;
	}

	/* conn is already encrypted against wiretappers, 
		but gf is also encrypted against server breakin. */
	if(buf == nil && (fd =create(gf, OWRITE, 0600)) < 0){
		fprint(2, "can't open %s: %r\n", gf);
		return -1;
	}

	ibr = ibw = ib;
	for(nr=0; nr < len;){
		if((n = conn->read(conn, ibw, Maxmsg)) <= 0){
			fprint(2, "empty file chunk n=%d nr=%d len=%d: %r\n", n, nr, len);
			return -1;
		}
		nr += n;
		ibw += n;
		if(!aes.setup){ /* first time, read 16 byte IV */
			if(n < AESbsize){
				fprint(2, "no IV in file\n");
				return -1;
			}
			sha = sha1((uchar*)"aescbc file", 11, nil, nil);
			sha1(key, nkey, skey, sha);
			setupAESstate(&aes, skey, AESbsize, ibr);
			memset(skey, 0, sizeof skey);
			ibr += AESbsize;
			n -= AESbsize;
		}
		aesCBCdecrypt(ibw-n, n, &aes);
		n = ibw-ibr-CHK;
		if(n > 0){
			if(buf == nil){
				nw = write(fd, ibr, n);
				if(nw != n){
					fprint(2, "write error on %s", gf);
					return -1;
				}
			}else{
				assert(bufw+n <= bufe);
				memmove(bufw, ibr, n);
				bufw += n;
			}
			ibr += n;
		}
		memmove(ib, ibr, ibw-ibr);
		ibw = ib + (ibw-ibr);
		ibr = ib;
	}
	if(buf == nil)
		close(fd);
	n = ibw-ibr;
	if((n != CHK) || (memcmp(ib, "XXXXXXXXXXXXXXXX", CHK) != 0)){
			fprint(2,"decrypted file failed to authenticate!\n");
			return -1;
	}
	return 0;
}

// This sends a file to the secstore disk that can, in an emergency, be
// decrypted by the program aescbc.c with HEX=sha1("aescbc file"+passphrase).
static int
putfile(SConn *conn, char *pf, uchar *buf, ulong len, uchar *key, int nkey)
{
	int i, n, fd, ivo, bufi, done;
	char s[Maxmsg];
	uchar  skey[SHA1dlen], b[CHK+Maxmsg], IV[AESbsize];
	AESstate aes;
	DigestState *sha;

	/* create initialization vector */
	srand(time(0));  /* doesn't need to be unpredictable */
	for(i=0; i<AESbsize; i++)
		IV[i] = 0xff & rand();
	sha = sha1((uchar*)"aescbc file", 11, nil, nil);
	sha1(key, nkey, skey, sha);
	setupAESstate(&aes, skey, AESbsize, IV);
	memset(skey, 0, sizeof skey);

	snprint(s, Maxmsg, "PUT %s\n", pf);
	conn->write(conn, (uchar*)s, strlen(s));

	if(buf == nil){
		/* get file size */
		if((fd = open(pf, OREAD)) < 0){
			fprint(2, "can't open %s: %r\n", pf);
			return -1;
		}
		len = seek(fd, 0, 2);
		seek(fd, 0, 0);
	} else {
		fd = -1;
	}
	if(len > MAXFILESIZE){
		fprint(2, "implausible filesize %ld for %s\n", len, pf);
		return -1;
	}

	/* send file size */
	snprint(s, Maxmsg, "%ld", len+AESbsize+CHK);
	conn->write(conn, (uchar*)s, strlen(s));

	/* send IV and file+XXXXX in Maxmsg chunks */
	ivo = AESbsize;
	bufi = 0;
	memcpy(b, IV, ivo);
	for(done = 0; !done; ){
		if(buf == nil){
			n = read(fd, b+ivo, Maxmsg-ivo);
			if(n < 0){
				fprint(2, "read error on %s: %r\n", pf);
				return -1;
			}
		}else{
			if((n = len - bufi) > Maxmsg-ivo)	
				n = Maxmsg-ivo;
			memcpy(b+ivo, buf+bufi, n);
			bufi += n;
		}
		n += ivo;
		ivo = 0;
		if(n < Maxmsg){ /* EOF on input; append XX... */
			memset(b+n, 'X', CHK);
			n += CHK; // might push n>Maxmsg
			done = 1;
		}
		aesCBCencrypt(b, n, &aes);
		if(n > Maxmsg){
			assert(done==1);
			conn->write(conn, b, Maxmsg);
			n -= Maxmsg;
			memmove(b, b+Maxmsg, n);
		}
		conn->write(conn, b, n);
	}


	if(buf == nil)
		close(fd);
	fprint(2, "saved %ld bytes\n", len);

	return 0;
}

static int
removefile(SConn *conn, char *rf)
{
	char buf[Maxmsg];

	if(strchr(rf, '/')){
		fprint(2, "simple filenames, not paths like %s\n", rf);
		return -1;
	}

	snprint(buf, Maxmsg, "RM %s\n", rf);
	conn->write(conn, (uchar*)buf, strlen(buf));

	return 0;
}

static int
login(char *id, char *dest, int chpass, char **gf, int *Gflag, char **pf, char **rf)
{
	char pass[64];
	ulong len;
	int rv = -1, fd, passlen, newpasslen = 0;
	uchar *memfile, *memcur, *memnext;
	char *S, *list, *cur, *next, *newpass = nil, *hexHi;
	char *f[8], s[Maxmsg+1], prompt[128], buf[Maxmsg];
	mpint *H = mpnew(0), *Hi = mpnew(0);
	SConn *conn;

	if(dest == nil){
		fprint(2, "tried to login with nil dest\n");
		exits("nil dest");
	}
	while(1){
		if((fd = dial(dest, nil, nil, nil)) < 0){
			fprint(2, "can't dial %s\n", dest);
			return -1;
		}
		if((conn = newSConn(fd)) == nil)
			return -1;
		getpasswd("secstore password: ", pass, sizeof pass);
		if(pass[0]==0){
			fprint(2, "null password, skipping secstore login\n");
			exits("no password");
			return -1;
		}
		if(PAKclient(conn, id, pass, &S) >= 0)
			break;
		conn->free(conn);
		// and let user try retyping the password
	}
	passlen = strlen(pass);
	fprint(2, "%s\n", S);
	if(readstr(conn, s) < 0)
		goto Out;
	if(strcmp(s, "STA") == 0){
		long sn;
		getpasswd("STA PIN+SecureID: ", s+3, (sizeof s)-3);
		sn = strlen(s+3);
		if(verbose)
			fprint(2, "%ld\n", sn);
		conn->write(conn, (uchar*)s, sn+3);
		readstr(conn, s);
	}
	if(strcmp(s, "OK") !=0){
		fprint(2, "%s\n", s);
		goto Out;
	}

	if(chpass == 0){ // normal case
		while(*gf != nil){
			if(verbose)
				fprint(2, "get %s\n", *gf);
			if(getfile(conn, *gf, *Gflag ? &memfile : nil, &len, (uchar*)pass, passlen) < 0)
				goto Out;
			if(*Gflag){
				// write one line at a time, as required by /mnt/factotum/ctl
				memcur = memfile;
				while(len>0){
					memnext = (uchar*)strchr((char*)memcur, '\n');
					if(memnext){
						write(1, memcur, memnext-memcur+1);
						len -= memnext-memcur+1;
						memcur = memnext+1;
					}else{
						write(1, memcur, len);
						break;
					}
				}
				free(memfile);
			}
			gf++;
			Gflag++;
		}
		while(*pf != nil){
			if(verbose)
				fprint(2, "put %s\n", *pf);
			if(putfile(conn, *pf, nil, 0, (uchar*)pass, passlen) < 0)
				goto Out;
			pf++;
		}
		while(*rf != nil){
			if(verbose)
				fprint(2, "rm  %s\n", *rf);
			if(removefile(conn, *rf) < 0)
				goto Out;
			rf++;
		}
	}else{	// changing our password is vulnerable to connection failure
		for(;;){
			snprint(prompt, sizeof(prompt), "new password for %s: ", id);
			if(getpasswd(prompt, buf, sizeof(buf)) < 0)
				goto Out;
			if(strlen(buf) >= 7)
				break;
			else if(strlen(buf) == 0){
				fprint(2, "!password change aborted\n");
				goto Out;
			}
			print("!password must be at least 7 characters\n");
		}
		newpass = estrdup(buf);
		newpasslen = strlen(newpass);
		snprint(prompt, sizeof(prompt), "retype password: ");
		if(getpasswd(prompt, buf, sizeof(buf)) < 0){
			fprint(2, "getpasswd failed\n");
			goto Out;
		}
		if(strcmp(buf, newpass) != 0){
			fprint(2, "passwords didn't match\n");
			goto Out;
		}

		conn->write(conn, (uchar*)"CHPASS", strlen("CHPASS"));
		hexHi = PAK_Hi(id, newpass, H, Hi);
		conn->write(conn, (uchar*)hexHi, strlen(hexHi));
		free(hexHi);
		mpfree(H);
		mpfree(Hi);

		if(getfile(conn, ".", (uchar **) &list, &len, nil, 0) < 0){
			fprint(2, "directory listing failed.\n");
			goto Out;
		}

		/* Loop over files and reencrypt them; try to keep going after error */
		for(cur=list; (next=strchr(cur, '\n')) != nil; cur=next+1){
			*next = '\0';
			if(tokenize(cur, f, nelem(f))< 1)
				break;
			fprint(2, "reencrypting '%s'\n", f[0]);
			if(getfile(conn, f[0], &memfile, &len, (uchar*)pass, passlen) < 0){
				fprint(2, "getfile of '%s' failed\n", f[0]);
				continue;
			}
			if(putfile(conn, f[0], memfile, len, (uchar*)newpass, newpasslen) < 0)
				fprint(2, "putfile of '%s' failed\n", f[0]);
			free(memfile);
		}
		free(list);
		free(newpass);
	}

	conn->write(conn, (uchar*)"BYE", 3);
	rv = 0;

Out:
	if(newpass != nil){
		memset(newpass, 0, newpasslen);
		free(newpass);
	}
	conn->free(conn);
	return rv;
}

int
main(int argc, char **argv)
{
	int chpass = 0, rc;
	int ngfile = 0, npfile = 0, nrfile = 0, Gflag[MAXFILES+1];
	char *gfile[MAXFILES], *pfile[MAXFILES], *rfile[MAXFILES];
	char *serve, *tcpserve, *user;

	serve = "$auth";
	user = getenv("user");
	memset(Gflag, 0, sizeof Gflag);

	ARGBEGIN{
	case 'c':
		chpass = 1;
		break;
	case 'G':
		Gflag[ngfile]++;
		/* fall through */
	case 'g':
		if(ngfile >= MAXFILES)
			exits("too many gfiles");
		gfile[ngfile++] = ARGF();
		if(gfile[ngfile-1] == nil)
			exits("usage");
		break;
	case 'p':
		if(npfile >= MAXFILES)
			exits("too many pfiles");
		pfile[npfile++] = ARGF();
		if(pfile[npfile-1] == nil)
			exits("usage");
		break;
	case 'r':
		if(nrfile >= MAXFILES)
			exits("too many rfiles");
		rfile[nrfile++] = ARGF();
		if(rfile[nrfile-1] == nil)
			exits("usage");
		break;
	case 's':
		serve = EARGF(usage());
		break;
	case 'u':
		user = EARGF(usage());
		break;
	case 'v':
		verbose++;
		break;
	}ARGEND;
	gfile[ngfile] = nil;
	pfile[npfile] = nil;
	rfile[nrfile] = nil;

	if(argc!=0 || user==nil)
		usage();

	if(chpass && (ngfile || npfile || nrfile)){
		fprint(2, "Get, put, and remove invalid with password change.\n");
		exits("usage");
	}

	rc = strlen(serve)+sizeof("tcp!!99990");
	tcpserve = malloc(rc);
	if(!tcpserve)
		exits("out of memory");
	else if(strchr(serve,'!'))
		strcpy(tcpserve, serve);
	else
		snprint(tcpserve, rc, "tcp!%s!5356", serve);
	rc = login(user, tcpserve, chpass, gfile, Gflag, pfile, rfile);
	free(tcpserve);
	if(rc < 0){
		fprint(2, "secstore failed\n");
		exits("secstore failed");
	}
	exits("");
	return 0;
}

