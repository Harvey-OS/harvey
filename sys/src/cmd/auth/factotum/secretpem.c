// load PEM-format RSA private key into factotum

#include <u.h>
#include <libc.h>
#include <auth.h>
#include <mp.h>
#include <libsec.h>

char*
readfile(char *name)
{
	int fd;
	char *s;
	Dir *d;

	fd = open(name, OREAD);
	if(fd < 0)
		return nil;
	if((d = dirfstat(fd)) == nil)
		return nil;
	s = malloc(d->length + 1);
	if(s == nil || readn(fd, s, d->length) != d->length){
		free(s);
		free(d);
		close(fd);
		return nil;
	}
	close(fd);
	s[d->length] = '\0';
	free(d);
	return s;
}

int
factotum_rsa_setpriv(int fd, char *keypem)
{
	int len, n;
	char *pem, *factotum;
	uchar *binary;
	RSApriv *p;

	pem = readfile(keypem);
	if(pem == nil){
		werrstr("can't read %s", keypem);
		return -1;
	}
	binary = decodepem(pem, "RSA PRIVATE KEY", &len);
	free(pem);
	if(binary == nil){
		werrstr("can't parse %s", keypem);
		return -1;
	}
	p = asn1toRSApriv(binary, len);
	free(binary);
	if(p == nil){
		werrstr("bad rsa private key in %s", keypem);
		return -1;
	}

	factotum = smprint("key proto=sshrsa size=%d ek=%B n=%B"
		" !dk=%B !p=%B !q=%B !kp=%B !kq=%B !c2=%B\n",
		mpsignif(p->pub.n), p->pub.ek, p->pub.n,
		p->dk, p->p, p->q, p->kp, p->kq, p->c2);
	len = strlen(factotum);
	rsaprivfree(p);
	if(factotum == nil)
		return -1;
	n = write(fd, factotum, len);
	free(factotum);
	if(n != len){
		werrstr("write error %d %d", len, n);
		return -1;
	}
	return 0;
}

void
main(int argc, char **argv)
{
	if(argc != 2 || argv[1][0]=='-'){
		fprint(2, "usage: %s key.pem > /mnt/factotum/ctl\n", argv[0]);
		exits("usage");
	}
	fmtinstall('B', mpfmt);
	if(factotum_rsa_setpriv(1, argv[1]) != 0)
		sysfatal("%s: %r", argv[0]);
	exits(0);
}
