#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <libsec.h>
#include "imap4d.h"

static int	saveMsg(char *dst, char *digest, int flags, char *head, int nhead, Biobuf *b, long n);
static int	saveb(int fd, DigestState *dstate, char *buf, int nr, int nw);
static long	appSpool(Biobuf *bout, Biobuf *bin, long n);

/*
 * check if the message exists
 */
int
copyCheck(Box *box, Msg *m, int uids, void *v)
{
	int fd;

	USED(box);
	USED(uids);
	USED(v);

	if(m->expunged)
		return 0;
	fd = msgFile(m, "raw");
	if(fd < 0){
		msgDead(m);
		return 0;
	}
	close(fd);
	return 1;
}

int
copySave(Box *box, Msg *m, int uids, void *vs)
{
	Dir *d;
	Biobuf b;
	vlong length;
	char *head;
	int ok, hfd, bfd, nhead;

	USED(box);
	USED(uids);

	if(m->expunged)
		return 0;

	hfd = msgFile(m, "unixheader");
	if(hfd < 0){
		msgDead(m);
		return 0;
	}
	head = readFile(hfd);
	if(head == nil){
		close(hfd);
		return 0;
	}

	/*
	 * clear out the header if it doesn't end in a newline,
	 * since it is a runt and the "header" will show up in the raw file.
	 */
	nhead = strlen(head);
	if(nhead > 0 && head[nhead-1] != '\n')
		nhead = 0;

	bfd = msgFile(m, "raw");
	close(hfd);
	if(bfd < 0){
		msgDead(m);
		return 0;
	}

	d = dirfstat(bfd);
	if(d == nil){
		close(bfd);
		return 0;
	}
	length = d->length;
	free(d);

	Binit(&b, bfd, OREAD);
	ok = saveMsg(vs, m->info[IDigest], m->flags, head, nhead, &b, length);
	Bterm(&b);
	close(bfd);
	return ok;
}

/*
 * first spool the input into a temorary file,
 * and massage the input in the process.
 * then save to real box.
 */
int
appendSave(char *mbox, int flags, char *head, Biobuf *b, long n)
{
	Biobuf btmp;
	int fd, ok;

	fd = imapTmp();
	if(fd < 0)
		return 0;
	Bprint(&bout, "+ Ready for literal data\r\n");
	if(Bflush(&bout) < 0)
		writeErr();
	Binit(&btmp, fd, OWRITE);
	n = appSpool(&btmp, b, n);
	Bterm(&btmp);
	if(n < 0){
		close(fd);
		return 0;
	}

	seek(fd, 0, 0);
	Binit(&btmp, fd, OREAD);
	ok = saveMsg(mbox, nil, flags, head, strlen(head), &btmp, n);
	Bterm(&btmp);
	close(fd);
	return ok;
}

/*
 * copy from bin to bout,
 * mapping "\r\n" to "\n" and "\nFrom " to "\n From "
 * return the number of bytes in the mapped file.
 *
 * exactly n bytes must be read from the input,
 * unless an input error occurs.
 */
static long
appSpool(Biobuf *bout, Biobuf *bin, long n)
{
	int i, c;

	c = '\n';
	while(n > 0){
		if(c == '\n' && n >= STRLEN("From ")){
			for(i = 0; i < STRLEN("From "); i++){
				c = Bgetc(bin);
				if(c != "From "[i]){
					if(c < 0)
						return -1;
					Bungetc(bin);
					break;
				}
				n--;
			}
			if(i == STRLEN("From "))
				Bputc(bout, ' ');
			Bwrite(bout, "From ", i);
		}
		c = Bgetc(bin);
		n--;
		if(c == '\r' && n-- > 0){
			c = Bgetc(bin);
			if(c != '\n')
				Bputc(bout, '\r');
		}
		if(c < 0)
			return -1;
		if(Bputc(bout, c) < 0)
			return -1;
	}
	if(c != '\n')
		Bputc(bout, '\n');
	if(Bflush(bout) < 0)
		return -1;
	return Boffset(bout);
}

static int
saveMsg(char *dst, char *digest, int flags, char *head, int nhead, Biobuf *b, long n)
{
	DigestState *dstate;
	MbLock *ml;
	uchar shadig[SHA1dlen];
	char buf[BufSize + 1], digbuf[NDigest + 1];
	int i, fd, nr, nw, ok;

	ml = mbLock();
	if(ml == nil)
		return 0;
	fd = openLocked(mboxDir, dst, OWRITE);
	if(fd < 0){
		mbUnlock(ml);
		return 0;
	}
	seek(fd, 0, 2);

	dstate = nil;
	if(digest == nil)
		dstate = sha1(nil, 0, nil, nil);
	if(!saveb(fd, dstate, head, nhead, nhead)){
		if(dstate != nil)
			sha1(nil, 0, shadig, dstate);
		mbUnlock(ml);
		close(fd);
		return 0;
	}
	ok = 1;
	if(n == 0)
		ok = saveb(fd, dstate, "\n", 0, 1);
	while(n > 0){
		nr = n;
		if(nr > BufSize)
			nr = BufSize;
		nr = Bread(b, buf, nr);
		if(nr <= 0){
			saveb(fd, dstate, "\n\n", 0, 2);
			ok = 0;
			break;
		}
		n -= nr;
		nw = nr;
		if(n == 0){
			if(buf[nw - 1] != '\n')
				buf[nw++] = '\n';
			buf[nw++] = '\n';
		}
		if(!saveb(fd, dstate, buf, nr, nw)){
			ok = 0;
			break;
		}
		mbLockRefresh(ml);
	}
	close(fd);

	if(dstate != nil){
		digest = digbuf;
		sha1(nil, 0, shadig, dstate);
		for(i = 0; i < SHA1dlen; i++)
			snprint(digest+2*i, NDigest+1-2*i, "%2.2ux", shadig[i]);
	}
	if(ok){
		fd = cdOpen(mboxDir, impName(dst), OWRITE);
		if(fd < 0)
			fd = emptyImp(dst);
		if(fd >= 0){
			seek(fd, 0, 2);
			wrImpFlags(buf, flags, 1);
			fprint(fd, "%.*s %.*lud %s\n", NDigest, digest, NUid, 0UL, buf);
			close(fd);
		}
	}
	mbUnlock(ml);
	return 1;
}

static int
saveb(int fd, DigestState *dstate, char *buf, int nr, int nw)
{
	if(dstate != nil)
		sha1((uchar*)buf, nr, nil, dstate);
	if(write(fd, buf, nw) != nw)
		return 0;
	return 1;
}
