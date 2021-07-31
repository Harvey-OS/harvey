#include <u.h>
#include <libc.h>

#include "../ssh.h"
#include "client.h"

#include <auth.h>
#include <fcall.h>

typedef struct Keydir Keydir;
struct Keydir
{
	char *name;
	int fd;
};

struct Keyring
{
	/* key file */
	FILE *fin;
	RSApriv *priv;

	/* key dir */
	mpint *pub;
	Keydir *dir;
	int ndir;
	int mdir;
	int keyfd;
	char *keypath;
};

Keyring*
openkeyringfile(char *file)
{
	Keyring *k;
	FILE *fin;

	if((fin = fopen(file, "r")) == nil)
		return nil;

	k = zalloc(sizeof *k);
	k->fin = fin;
	k->priv = rsaprivalloc();
	return k;
}

static char*
estrdup(char *s)
{
	char *t;

	t = zalloc(strlen(s)+1);
	strcpy(t, s);
	return t;
}

static char*
estrpath(char *dir, char *file)
{
	char *t;

	t = zalloc(strlen(dir)+1+strlen(file)+1);
	strcpy(t, dir);
	strcat(t, "/");
	strcat(t, file);
	return t;
}

Keyring*
openkeyringdir(char *dir)
{
	int fd;
	Keyring *k;

	if((fd = open(dir, OREAD)) < 0)
		return nil;

	k = zalloc(sizeof *k);
	k->dir = zalloc(sizeof k->dir[0]);
	k->ndir = 1;
	k->mdir = 1;
	k->dir[0].fd = fd;
	k->dir[0].name = estrdup(dir);
	k->keyfd = -1;
	return k;
}

static mpint*
readmpint(int fd)
{
	char buf[8192];
	int n;

	n = read(fd, buf, sizeof buf);
	if(n <= 0)
		return nil;

	if(n >= sizeof buf)
		n = sizeof(buf)-1;
	buf[n] = '\0';

	return strtomp(buf, nil, 16, nil);
}

static int
writempint(int fd, mpint *m)
{
	char *p;
	int r;

	p = mptoa(m, 16, nil, 0);
	if(p == nil)
		return -1;

	r = write(fd, p, strlen(p))==strlen(p) ? 0 : -1;
	free(p);
	return r;
}

static int
trykeyfd(Keyring *k)
{
	char buf[DIRLEN], *p;
	int fd;
	Dir d;

	if(k->ndir == 0)
		return -1;

	/* read next entry from directory */
	if(read(k->dir[k->ndir-1].fd, buf, sizeof buf) <= 0 || convM2D(buf, &d) < 0){
		close(k->dir[k->ndir-1].fd);
		free(k->dir[k->ndir-1].name);
		k->ndir--;
		return -1;
	}

	p = estrpath(k->dir[k->ndir-1].name, d.name);

	/* if it's a file, return it */
	if((d.mode & CHDIR) == 0){
		if((fd = open(p, ORDWR)) < 0){
			free(p);
			return -1;
		}
		free(k->keypath);
		k->keypath = p;
		return fd;
	}

	/* it's a directory, open it */
	if((fd = open(p, OREAD)) < 0){
		free(p);
		return -1;
	}
	if(k->ndir >= k->mdir){
		k->dir = realloc(k->dir, (k->ndir+1)*sizeof(k->dir[0]));
		if(k->dir == nil)
			error("out of memory reallocing");
	}
	k->dir[k->ndir].name = p;
	k->dir[k->ndir].fd = fd;
	k->ndir++;
	return -1;
}

static int
nextkeyfd(Keyring *k)
{
	int fd;

	while(k->ndir > 0)
		if((fd = trykeyfd(k)) >= 0)
			return fd;
	
	return -1;
}

mpint*
nextkey(Keyring *k)
{
	/* file */
	if(k->priv){
		rsaprivfree(k->priv);
		k->priv = rsaprivalloc();
		if(readsecretkey(k->fin, k->priv) == 0)
			return nil;
		return k->priv->pub.n;
	}

	/* agent */
	if(k->keyfd >= 0)
		close(k->keyfd);
	if(k->pub){
		mpfree(k->pub);
		k->pub = nil;
	}

	for(;;){
		if((k->keyfd = nextkeyfd(k)) < 0)
			return nil;
		if(k->pub = readmpint(k->keyfd))
			return k->pub;
		close(k->keyfd);
		k->keyfd = -1;
	}
	return nil;	/* notreached */
}

mpint*
keydecrypt(Keyring *k, mpint *chal)
{
	mpint *resp;

	/* file */
	if(k->priv)
		return RSADecrypt(chal, k->priv);

	/* agent */
	if(writempint(k->keyfd, chal) < 0
	|| (resp = readmpint(k->keyfd)) == nil)
		return nil;
	mpfree(chal);
	return resp;
}

void
closekeyring(Keyring *k)
{
	if(k->priv)
		rsaprivfree(k->priv);
	if(k->fin)
		fclose(k->fin);
	free(k);
}

