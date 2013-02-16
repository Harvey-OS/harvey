#include <u.h>
#include <libc.h>
#include <mp.h>
#include <ctype.h>
#include <libsec.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "netssh.h"

enum {
	Arbsz =	256,
};

static int
parsepubkey(char *s, RSApub *key, char **sp, int base)
{
	int n;
	char *host, *p, *z;

	z = nil;
	n = strtoul(s, &p, 10);
	host = nil;
	if(n < Arbsz || !isspace(*p)){		/* maybe this is a host name */
		host = s;
		s = strpbrk(s, " \t");
		if(s == nil)
			return -1;
		z = s;
		*s++ = '\0';
		s += strspn(s, " \t");

		n = strtoul(s, &p, 10);
		if(n < Arbsz || !isspace(*p)){
			if(z)
				*z = ' ';
			return -1;
		}
	}

	/* Arbsz is just a sanity check */
	if((key->ek = strtomp(p, &p, base, nil)) == nil ||
	    (key->n = strtomp(p, &p, base, nil)) == nil ||
	    (*p != '\0' && !isspace(*p)) || mpsignif(key->n) < Arbsz) {
		mpfree(key->ek);
		mpfree(key->n);
		key->ek = nil;
		key->n = nil;
		if(z)
			*z = ' ';
		return -1;
	}
	if(host == nil){
		if(*p != '\0'){
			p += strspn(p, " \t");
			if(*p != '\0'){
				host = emalloc9p(strlen(p)+1);
				strcpy(host, p);
			}
		}
		free(s);
	}
	*sp = host;
	return 0;
}

RSApub*
readpublickey(Biobuf *b, char **sp)
{
	char *s;
	RSApub *key;

	key = emalloc9p(sizeof(RSApub));
	if(key == nil)
		return nil;

	for (; (s = Brdstr(b, '\n', 1)) != nil; free(s))
		if(s[0] != '#'){
			if(parsepubkey(s, key, sp, 10) == 0 ||
			    parsepubkey(s, key, sp, 16) == 0)
				return key;
			fprint(2, "warning: skipping line '%s'; cannot parse\n",
				s);
		}
	free(key);
	return nil;
}

static int
match(char *pattern, char *aliases)
{
	char *s, *snext, *a, *anext, *ae;

	for(s = pattern; s && *s; s = snext){
		if((snext = strchr(s, ',')) != nil)
			*snext++ = '\0';
		for(a = aliases; a && *a; a = anext){
			if((anext = strchr(a, ',')) != nil){
				ae = anext;
				anext++;
			}else
				ae = a + strlen(a);
			if(ae - a == strlen(s) && memcmp(s, a, ae - a) == 0)
				return 0;
		}
	}
	return 1;
}

int
findkey(char *keyfile, char *host, RSApub *key)
{
	char *h;
	Biobuf *b;
	RSApub *k;
	int res;

	if ((b = Bopen(keyfile, OREAD)) == nil)
		return NoKeyFile;

	for (res = NoKey; res != KeyOk;) {
		if ((k = readpublickey(b, &h)) == nil)
			break;
		if (match(h, host) == 0) {
			if (mpcmp(k->n, key->n) == 0 &&
			    mpcmp(k->ek, key->ek) == 0)
				res = KeyOk;
			else
				res = KeyWrong;
		}
		free(h);
		free(k->ek);
		free(k->n);
		free(k);
	}
	Bterm(b);
	return res;
}

int
replacekey(char *keyfile, char *host, RSApub *hostkey)
{
	int ret;
	char *h, *nkey, *p;
	Biobuf *br, *bw;
	Dir *d, nd;
	RSApub *k;

	ret = -1;
	d = nil;
	nkey = smprint("%s.new", keyfile);
	if(nkey == nil)
		return -1;

	if((br = Bopen(keyfile, OREAD)) == nil)
		goto out;
	if((bw = Bopen(nkey, OWRITE)) == nil){
		Bterm(br);
		goto out;
	}

	while((k = readpublickey(br, &h)) != nil){
		if(match(h, host) != 0)
			Bprint(bw, "%s %d %.10M %.10M\n",
				h, mpsignif(k->n), k->ek, k->n);
		free(h);
		rsapubfree(k);
	}
	Bprint(bw, "%s %d %.10M %.10M\n", host, mpsignif(hostkey->n),
		hostkey->ek, hostkey->n);
	Bterm(bw);
	Bterm(br);

	d = dirstat(nkey);
	if(d == nil){
		fprint(2, "new key file disappeared?\n");
		goto out;
	}

	p = strrchr(d->name, '.');
	if(p == nil || strcmp(p, ".new") != 0){
		fprint(2, "%s: new key file changed names? %s to %s\n",
			argv0, nkey, d->name);
		goto out;
	}

	*p = '\0';
	nulldir(&nd);
	nd.name = d->name;
	if(remove(keyfile) < 0){
		fprint(2, "%s: error removing %s: %r\n", argv0, keyfile);
		goto out;
	}
	if(dirwstat(nkey, &nd) < 0){
		fprint(2, "%s: error renaming %s to %s: %r\n",
			argv0, nkey, d->name);
		goto out;
	}
	ret = 0;
out:
	free(d);
	free(nkey);
	return ret;
}

int
appendkey(char *keyfile, char *host, RSApub *key)
{
	int fd, ret;

	ret = -1;
	if((fd = open(keyfile, OWRITE)) < 0){
		fd = create(keyfile, OWRITE, 0666);
		if(fd < 0){
			fprint(2, "%s: can't open nor create %s: %r\n",
				argv0, keyfile);
			return -1;
		}
	}
	if(seek(fd, 0, 2) >= 0 &&
	    fprint(fd, "%s %d %.10M %.10M\n", host, mpsignif(key->n),
	     key->ek, key->n) >= 0)
		ret = 0;
	close(fd);
	return ret;
}
