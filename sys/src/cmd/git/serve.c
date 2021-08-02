#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <auth.h>

#include "git.h"

char	*pathpfx = nil;
int	allowwrite;

int
fmtpkt(Conn *c, char *fmt, ...)
{
	char pkt[Pktmax];
	va_list ap;
	int n;

	va_start(ap, fmt);
	n = vsnprint(pkt, sizeof(pkt), fmt, ap);
	n = writepkt(c, pkt, n);
	va_end(ap);
	return n;
}

int
showrefs(Conn *c)
{
	int i, ret, nrefs;
	Hash head, *refs;
	char **names;

	ret = -1;
	nrefs = 0;
	refs = nil;
	names = nil;
	if(resolveref(&head, "HEAD") != -1)
		if(fmtpkt(c, "%H HEAD", head) == -1)
			goto error;

	if((nrefs = listrefs(&refs, &names)) == -1)
		sysfatal("listrefs: %r");
	for(i = 0; i < nrefs; i++){
		if(strncmp(names[i], "heads/", strlen("heads/")) != 0)
			continue;
		if(fmtpkt(c, "%H refs/%s\n", refs[i], names[i]) == -1)
			goto error;
	}
	if(flushpkt(c) == -1)
		goto error;
	ret = 0;
error:
	for(i = 0; i < nrefs; i++)
		free(names[i]);
	free(names);
	free(refs);
	return ret;
}

int
servnegotiate(Conn *c, Hash **head, int *nhead, Hash **tail, int *ntail)
{
	char pkt[Pktmax];
	int n, acked;
	Object *o;
	Hash h;

	if(showrefs(c) == -1)
		return -1;

	*head = nil;
	*tail = nil;
	*nhead = 0;
	*ntail = 0;
	while(1){
		if((n = readpkt(c, pkt, sizeof(pkt))) == -1)
			goto error;
		if(n == 0)
			break;
		if(strncmp(pkt, "want ", 5) != 0){
			werrstr(" protocol garble %s", pkt);
			goto error;
		}
		if(hparse(&h, &pkt[5]) == -1){
			werrstr(" garbled want");
			goto error;
		}
		if((o = readobject(h)) == nil){
			werrstr("requested nonexistent object");
			goto error;
		}
		unref(o);
		*head = erealloc(*head, (*nhead + 1)*sizeof(Hash));
		(*head)[*nhead] = h;	
		*nhead += 1;
	}

	acked = 0;
	while(1){
		if((n = readpkt(c, pkt, sizeof(pkt))) == -1)
			goto error;
		if(strncmp(pkt, "done", 4) == 0)
			break;
		if(n == 0){
			if(!acked && fmtpkt(c, "NAK") == -1)
					goto error;
		}
		if(strncmp(pkt, "have ", 5) != 0){
			werrstr(" protocol garble %s", pkt);
			goto error;
		}
		if(hparse(&h, &pkt[5]) == -1){
			werrstr(" garbled have");
			goto error;
		}
		if((o = readobject(h)) == nil)
			continue;
		if(!acked){
			if(fmtpkt(c, "ACK %H", h) == -1)
				goto error;
			acked = 1;
		}
		unref(o);
		*tail = erealloc(*tail, (*ntail + 1)*sizeof(Hash));
		(*tail)[*ntail] = h;	
		*ntail += 1;
	}
	if(!acked && fmtpkt(c, "NAK\n") == -1)
		goto error;
	return 0;
error:
	fmtpkt(c, "ERR %r\n");
	free(*head);
	free(*tail);
	return -1;
}

int
servpack(Conn *c)
{
	Hash *head, *tail, h;
	int nhead, ntail;

	dprint(1, "negotiating pack\n");
	if(servnegotiate(c, &head, &nhead, &tail, &ntail) == -1)
		sysfatal("negotiate: %r");
	dprint(1, "writing pack\n");
	if(writepack(c->wfd, head, nhead, tail, ntail, &h) == -1)
		sysfatal("send: %r");
	return 0;
}

int
validref(char *s)
{
	if(strncmp(s, "refs/", 5) != 0)
		return 0;
	for(; *s != '\0'; s++)
		if(!isalnum(*s) && strchr("/-_.", *s) == nil)
			return 0;
	return 1;
}

int
recvnegotiate(Conn *c, Hash **cur, Hash **upd, char ***ref, int *nupd)
{
	char pkt[Pktmax], *sp[4];
	Hash old, new;
	int n, i;

	if(showrefs(c) == -1)
		return -1;
	*cur = nil;
	*upd = nil;
	*ref = nil;
	*nupd = 0;
	while(1){
		if((n = readpkt(c, pkt, sizeof(pkt))) == -1)
			goto error;
		if(n == 0)
			break;
		if(getfields(pkt, sp, nelem(sp), 1, " \t\n\r") != 3){
			fmtpkt(c, "ERR  protocol garble %s\n", pkt);
			goto error;
		}
		if(hparse(&old, sp[0]) == -1){
			fmtpkt(c, "ERR bad old hash %s\n", sp[0]);
			goto error;
		}
		if(hparse(&new, sp[1]) == -1){
			fmtpkt(c, "ERR bad new hash %s\n", sp[1]);
			goto error;
		}
		if(!validref(sp[2])){
			fmtpkt(c, "ERR invalid ref %s\n", sp[2]);
			goto error;
		}
		*cur = erealloc(*cur, (*nupd + 1)*sizeof(Hash));
		*upd = erealloc(*upd, (*nupd + 1)*sizeof(Hash));
		*ref = erealloc(*ref, (*nupd + 1)*sizeof(Hash));
		(*cur)[*nupd] = old;
		(*upd)[*nupd] = new;
		(*ref)[*nupd] = estrdup(sp[2]);
		*nupd += 1;
	}		
	return 0;
error:
	free(*cur);
	free(*upd);
	for(i = 0; i < *nupd; i++)
		free((*ref)[i]);
	free(*ref);
	return -1;
}

int
rename(char *pack, char *idx, Hash h)
{
	char name[128], path[196];
	Dir st;

	nulldir(&st);
	st.name = name;
	snprint(name, sizeof(name), "%H.pack", h);
	snprint(path, sizeof(path), ".git/objects/pack/%s", name);
	if(access(path, AEXIST) == 0)
		fprint(2, "warning, pack %s already pushed\n", name);
	else if(dirwstat(pack, &st) == -1)
		return -1;
	snprint(name, sizeof(name), "%H.idx", h);
	snprint(path, sizeof(path), ".git/objects/pack/%s", name);
	if(access(path, AEXIST) == 0)
		fprint(2, "warning, pack %s already indexed\n", name);
	else if(dirwstat(idx, &st) == -1)
		return -1;
	return 0;
}

int
checkhash(int fd, vlong sz, Hash *hcomp)
{
	DigestState *st;
	Hash hexpect;
	char buf[Pktmax];
	vlong n, r;
	int nr;
	
	if(sz < 28){
		werrstr("undersize packfile");
		return -1;
	}

	st = nil;
	n = 0;
	if(seek(fd, 0, 0) == -1)
		sysfatal("packfile seek: %r");
	while(n != sz - 20){
		nr = sizeof(buf);
		if(sz - n - 20 < sizeof(buf))
			nr = sz - n - 20;
		r = readn(fd, buf, nr);
		if(r != nr){
			werrstr("short read");
			return -1;
		}
		st = sha1((uchar*)buf, nr, nil, st);
		n += r;
	}
	sha1(nil, 0, hcomp->h, st);
	if(readn(fd, hexpect.h, sizeof(hexpect.h)) != sizeof(hexpect.h))
		sysfatal("truncated packfile");
	if(!hasheq(hcomp, &hexpect)){
		werrstr("bad hash: %H != %H", *hcomp, hexpect);
		return -1;
	}
	return 0;
}

int
mkdir(char *dir)
{
	char buf[ERRMAX];
	int f;

	if(access(dir, AEXIST) == 0)
		return 0;
	if((f = create(dir, OREAD, DMDIR | 0755)) == -1){
		rerrstr(buf, sizeof(buf));
		if(strstr(buf, "exist") == nil)
			return -1;
	}
	close(f);
	return 0;
}

int
updatepack(Conn *c)
{
	char buf[Pktmax], packtmp[128], idxtmp[128], ebuf[ERRMAX];
	int n, pfd, packsz;
	Hash h;

	/* make sure the needed dirs exist */
	if(mkdir(".git/objects") == -1)
		return -1;
	if(mkdir(".git/objects/pack") == -1)
		return -1;
	if(mkdir(".git/refs") == -1)
		return -1;
	if(mkdir(".git/refs/heads") == -1)
		return -1;
	snprint(packtmp, sizeof(packtmp), ".git/objects/pack/recv-%d.pack.tmp", getpid());
	snprint(idxtmp, sizeof(idxtmp), ".git/objects/pack/recv-%d.idx.tmp", getpid());
	if((pfd = create(packtmp, ORDWR, 0644)) == -1)
		return -1;
	packsz = 0;
	while(1){
		n = read(c->rfd, buf, sizeof(buf));
		if(n == 0)
			break;
		if(n == -1){
			rerrstr(ebuf, sizeof(ebuf));
			if(strstr(ebuf, "hungup") == nil)
				return -1;
			break;
		}
		if(write(pfd, buf, n) != n)
			return -1;
		packsz += n;
	}
	if(checkhash(pfd, packsz, &h) == -1){
		dprint(1, "hash mismatch\n");
		goto error1;
	}
	if(indexpack(packtmp, idxtmp, h) == -1){
		dprint(1, "indexing failed: %r\n");
		goto error1;
	}
	if(rename(packtmp, idxtmp, h) == -1){
		dprint(1, "rename failed: %r\n");
		goto error2;
	}
	return 0;

error2:	remove(idxtmp);
error1:	remove(packtmp);
	return -1;
}	

int
lockrepo(void)
{
	int fd, i;

	for(i = 0; i < 10; i++) {
		if((fd = create(".git/_lock", ORCLOSE|ORDWR|OTRUNC|OEXCL, 0644))!= -1)
			return fd;
		sleep(250);
	}
	return -1;
}

int
updaterefs(Conn *c, Hash *cur, Hash *upd, char **ref, int nupd)
{
	char refpath[512];
	int i, newidx, hadref, fd, ret, lockfd;
	vlong newtm;
	Object *o;
	Hash h;

	ret = -1;
	hadref = 0;
	newidx = -1;
	/*
	 * Date of Magna Carta.
	 * Wrong because it  was computed using
	 * the proleptic gregorian calendar.
	 */
	newtm = -23811206400;	
	if((lockfd = lockrepo()) == -1){
		werrstr("repo locked\n");
		return -1;
	}
	for(i = 0; i < nupd; i++){
		if(resolveref(&h, ref[i]) == 0){
			hadref = 1;
			if(!hasheq(&h, &cur[i])){
				werrstr("old ref changed: %s", ref[i]);
				goto error;
			}
		}
		if(snprint(refpath, sizeof(refpath), ".git/%s", ref[i]) == sizeof(refpath)){
			werrstr("ref path too long: %s", ref[i]);
			goto error;
		}
		if(hasheq(&upd[i], &Zhash)){
			remove(refpath);
			continue;
		}
		if((o = readobject(upd[i])) == nil){
			werrstr("update to nonexistent hash %H", upd[i]);
			goto error;
		}
		if(o->type != GCommit){
			werrstr("not commit: %H", upd[i]);
			goto error;
		}
		if(o->commit->mtime > newtm){
			newtm = o->commit->mtime;
			newidx = i;
		}
		unref(o);
		if((fd = create(refpath, OWRITE|OTRUNC, 0644)) == -1){
			werrstr("open ref: %r");
			goto error;
		}
		if(fprint(fd, "%H", upd[i]) == -1){
			werrstr("upate ref: %r");
			close(fd);
			goto error;
		}
		close(fd);
	}
	/*
	 * Heuristic:
	 * If there are no valid refs, and HEAD is invalid, then
	 * pick the ref with the newest commits as the default
	 * branch.
	 *
	 * Several people have been caught out by pushing to
	 * a repo where HEAD named differently from what got
	 * pushed, and this is going to be more of a footgun
	 * when 'master', 'main', and 'front' are all in active
	 * use. This should make us pick a useful default in
	 * those cases, instead of silently failing.
	 */
	if(resolveref(&h, "HEAD") == -1 && hadref == 0 && newidx != -1){
		if((fd = create(".git/HEAD", OWRITE|OTRUNC, 0644)) == -1){
			werrstr("open HEAD: %r");
			goto error;
		}
		if(fprint(fd, "ref: %s", ref[0]) == -1){
			werrstr("write HEAD ref: %r");
			goto error;
		}
		close(fd);
	}
	ret = 0;
error:
	fmtpkt(c, "ERR %r");
	close(lockfd);
	return ret;
}

int
recvpack(Conn *c)
{
	Hash *cur, *upd;
	char **ref;
	int nupd;

	if(recvnegotiate(c, &cur, &upd, &ref, &nupd) == -1)
		sysfatal("negotiate refs: %r");
	if(nupd != 0 && updatepack(c) == -1)
		sysfatal("update pack: %r");
	if(nupd != 0 && updaterefs(c, cur, upd, ref, nupd) == -1)
		sysfatal("update refs: %r");
	return 0;
}

char*
parsecmd(char *buf, char *cmd, int ncmd)
{
	int i;
	char *p;

	for(p = buf, i = 0; *p && i < ncmd - 1; i++, p++){
		if(*p == ' ' || *p == '\t'){
			cmd[i] = 0;
			break;
		}
		cmd[i] = *p;
	}
	while(*p == ' ' || *p == '\t')
		p++;
	return p;
}

void
usage(void)
{
	fprint(2, "usage: %s [-dw] [-r rel]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *repo, cmd[32], buf[512];
	Conn c;

	ARGBEGIN{
	case 'd':
		chattygit++;
		break;
	case 'r':
		pathpfx = EARGF(usage());
		if(*pathpfx != '/')
			sysfatal("path prefix must begin with '/'");
		break;
	case 'w':
		allowwrite++;
		break;
	default:
		usage();
		break;
	}ARGEND;

	gitinit();
	interactive = 0;
	if(rfork(RFNAMEG) == -1)
		sysfatal("rfork: %r");
	if(pathpfx != nil){
		if(bind(pathpfx, "/", MREPL) == -1)
			sysfatal("bind: %r");
	}
	if(rfork(RFNOMNT) == -1)
		sysfatal("rfork: %r");

	initconn(&c, 0, 1);
	if(readpkt(&c, buf, sizeof(buf)) == -1)
		sysfatal("readpkt: %r");
	repo = parsecmd(buf, cmd, sizeof(cmd));
	cleanname(repo);
	if(strncmp(repo, "../", 3) == 0)
		sysfatal("invalid path %s\n", repo);
	if(bind(repo, "/", MREPL) == -1){
		fmtpkt(&c, "ERR no repo %r\n");
		sysfatal("enter %s: %r", repo);
	}
	if(chdir("/") == -1)
		sysfatal("chdir: %r");
	if(access(".git", AREAD) == -1)
		sysfatal("no git repository");
	if(strcmp(cmd, "git-receive-pack") == 0 && allowwrite)
		recvpack(&c);
	else if(strcmp(cmd, "git-upload-pack") == 0)
		servpack(&c);
	else
		sysfatal("unsupported command '%s'", cmd);
	exits(nil);
}
