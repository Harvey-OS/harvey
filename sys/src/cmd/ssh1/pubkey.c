#include "ssh.h"
#include <bio.h>
#include <ctype.h>

static int
parsepubkey(char *s, RSApub *key, char **sp, int base)
{
	int n;
	char *host, *p, *z;

	z = nil;
	n = strtoul(s, &p, 10);
	host = nil;
	if(n < 256 || !isspace(*p)){	/* maybe this is a host name */
		host = s;
		s = strpbrk(s, " \t");
		if(s == nil)
			return -1;
		z = s;
		*s++ = '\0';
		s += strspn(s, " \t");

		n = strtoul(s, &p, 10);
		if(n < 256 || !isspace(*p)){
			if(z)
				*z = ' ';
			return -1;
		}
	}

	if((key->ek = strtomp(p, &p, base, nil)) == nil
	|| (key->n = strtomp(p, &p, base, nil)) == nil
	|| (*p != '\0' && !isspace(*p))
	|| mpsignif(key->n) < 256){	/* 256 is just a sanity check */
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
				host = emalloc(strlen(p)+1);
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

	key = rsapuballoc();
	if(key == nil)
		return nil;

	for(;;){
		if((s = Brdstr(b, '\n', 1)) == nil){
			rsapubfree(key);
			return nil;
		}
		if(s[0]=='#'){
			free(s);
			continue;
		}
		if(parsepubkey(s, key, sp, 10)==0
		|| parsepubkey(s, key, sp, 16)==0)
			return key;
		fprint(2, "warning: skipping line '%s'; cannot parse\n", s);
		free(s);
	}
}

static int
match(char *pattern, char *aliases)
{
	char *s, *snext;
	char *a, *anext, *ae;

	for(s=pattern; s && *s; s=snext){
		if((snext=strchr(s, ',')) != nil)
			*snext++ = '\0';
		for(a=aliases; a && *a; a=anext){
			if((anext=strchr(a, ',')) != nil){
				ae = anext;
				anext++;
			}else
				ae = a+strlen(a);
			if(ae-a == strlen(s) && memcmp(s, a, ae-a)==0)
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

	if((b = Bopen(keyfile, OREAD)) == nil)
		return NoKeyFile;

	for(;;){
		if((k = readpublickey(b, &h)) == nil){
			Bterm(b);
			return NoKey;
		}
		if(match(h, host) != 0){
			free(h);
			rsapubfree(k);
			continue;
		}
		if(mpcmp(k->n, key->n) != 0 || mpcmp(k->ek, key->ek) != 0){
			free(h);
			rsapubfree(k);
			Bterm(b);
			return KeyWrong;
		}
		free(h);
		rsapubfree(k);
		Bterm(b);
		return KeyOk;
	}
}

int
replacekey(char *keyfile, char *host, RSApub *hostkey)
{
	char *h, *nkey, *p;
	Biobuf *br, *bw;
	Dir *d, nd;
	RSApub *k;

	nkey = smprint("%s.new", keyfile);
	if(nkey == nil)
		return -1;

	if((br = Bopen(keyfile, OREAD)) == nil){
		free(nkey);
		return -1;
	}
	if((bw = Bopen(nkey, OWRITE)) == nil){
		Bterm(br);
		free(nkey);
		return -1;
	}

	while((k = readpublickey(br, &h)) != nil){
		if(match(h, host) != 0){
			Bprint(bw, "%s %d %.10B %.10B\n",
				h, mpsignif(k->n), k->ek, k->n);
		}
		free(h);
		rsapubfree(k);
	}
	Bprint(bw, "%s %d %.10B %.10B\n", host, mpsignif(hostkey->n), hostkey->ek, hostkey->n);
	Bterm(bw);
	Bterm(br);

	d = dirstat(nkey);
	if(d == nil){
		fprint(2, "new key file disappeared?\n");
		free(nkey);
		return -1;
	}

	p = strrchr(d->name, '.');
	if(p==nil || strcmp(p, ".new")!=0){
		fprint(2, "new key file changed names? %s to %s\n", nkey, d->name);
		free(d);
		free(nkey);
		return -1;
	}

	*p = '\0';
	nulldir(&nd);
	nd.name = d->name;
	if(remove(keyfile) < 0){
		fprint(2, "error removing %s: %r\n", keyfile);
		free(d);
		free(nkey);
		return -1;
	}
	if(dirwstat(nkey, &nd) < 0){
		fprint(2, "error renaming %s to %s: %r\n", nkey, d->name);
		free(nkey);
		free(d);
		return -1;
	}
	free(d);
	free(nkey);
	return 0;
}

int
appendkey(char *keyfile, char *host, RSApub *key)
{
	int fd;

	if((fd = open(keyfile, OWRITE)) < 0){
		fd = create(keyfile, OWRITE, 0666);
		if(fd < 0){
			fprint(2, "cannot open nor create %s: %r\n", keyfile);
			return -1;
		}
	}
	if(seek(fd, 0, 2) < 0
	|| fprint(fd, "%s %d %.10B %.10B\n", host, mpsignif(key->n), key->ek, key->n) < 0){
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}
