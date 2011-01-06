/*
 * sha1sum - compute SHA1 or SHA2 digest
 */
#include <u.h>
#include <libc.h>
#include <bio.h>
#include <libsec.h>

#pragma	varargck	type	"M"	uchar*

typedef struct Sha2 Sha2;
struct Sha2 {
	int	bits;
	int	dlen;
	DigestState* (*func)(uchar *, ulong, uchar *, DigestState *);
};

static Sha2 sha2s[] = {
	224,	SHA2_224dlen,	sha2_224,
	256,	SHA2_256dlen,	sha2_256,
	384,	SHA2_384dlen,	sha2_384,
	512,	SHA2_512dlen,	sha2_512,
};

static DigestState* (*shafunc)(uchar *, ulong, uchar *, DigestState *);
static int shadlen;

static int
digestfmt(Fmt *fmt)
{
	char buf[SHA2_512dlen*2 + 1];
	uchar *p;
	int i;

	p = va_arg(fmt->args, uchar*);
	for(i = 0; i < shadlen; i++)
		sprint(buf + 2*i, "%.2ux", p[i]);
	return fmtstrcpy(fmt, buf);
}

static void
sum(int fd, char *name)
{
	int n;
	uchar buf[8192], digest[SHA2_512dlen];
	DigestState *s;

	s = (*shafunc)(nil, 0, nil, nil);
	while((n = read(fd, buf, sizeof buf)) > 0)
		(*shafunc)(buf, n, nil, s);
	if(n < 0){
		fprint(2, "reading %s: %r\n", name? name: "stdin");
		return;
	}
	(*shafunc)(nil, 0, digest, s);
	if(name == nil)
		print("%M\n", digest);
	else
		print("%M\t%s\n", digest, name);
}

static void
usage(void)
{
	fprint(2, "usage: %s [-2 bits] [file...]\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int i, fd, bits;
	Sha2 *sha;

	shafunc = sha1;
	shadlen = SHA1dlen;
	ARGBEGIN{
	case '2':
		bits = atoi(EARGF(usage()));
		for (sha = sha2s; sha < sha2s + nelem(sha2s); sha++)
			if (sha->bits == bits)
				break;
		if (sha >= sha2s + nelem(sha2s))
			sysfatal("unknown number of sha2 bits: %d", bits);
		shafunc = sha->func;
		shadlen = sha->dlen;
		break;
	default:
		usage();
	}ARGEND

	fmtinstall('M', digestfmt);

	if(argc == 0)
		sum(0, nil);
	else
		for(i = 0; i < argc; i++){
			fd = open(argv[i], OREAD);
			if(fd < 0){
				fprint(2, "%s: can't open %s: %r\n", argv0, argv[i]);
				continue;
			}
			sum(fd, argv[i]);
			close(fd);
		}
	exits(nil);
}
