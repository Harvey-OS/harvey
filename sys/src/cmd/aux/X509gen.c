#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mp.h>
#include <libsec.h>

int PEMflag = 0;

void
main(int argc, char **argv)
{
	Biobuf *bin;
	int len, pemlen;
	char *keystr, *field[20], *buf;
	uchar *cert;
	RSApriv priv;
	ulong valid[2];

	valid[0] = time(0);
	valid[1] = valid[0] + 3*366*24*60*60;
	ARGBEGIN{
	case 'e':
		valid[1] = valid[0] + strtoul(ARGF(), 0, 10);
		break;
	case 'p':
		PEMflag = 1;
		break;
	}ARGEND
	fmtinstall('B', mpfmt);
	fmtinstall('H', encodefmt);

	if(argc<2){
		fprint(2, "usage: aux/X509gen key.secret 'C=US ...CN=xxx' > cert");
		exits("X509gen usage");
	}
	bin = Bopen(argv[0], OREAD);
	if(bin == nil){
		fprint(2, "can't open %s", argv[0]);
		exits("gen_x509 open key.secret");
	}
	keystr = Brdstr(bin, '\n', 1);
	Bterm(bin);
	if(tokenize(keystr, field, nelem(field)) != 9){
		fprint(2, "expected 9 fields in %s", argv[0]);
		exits("gen_x509 fields");
	}
	priv.pub.ek	= strtomp(field[1], nil, 16, nil);
	priv.dk		= strtomp(field[2], nil, 16, nil);
	priv.pub.n	= strtomp(field[3], nil, 16, nil);
	priv.p		= strtomp(field[4], nil, 16, nil);
	priv.q		= strtomp(field[5], nil, 16, nil);
	priv.kp		= strtomp(field[6], nil, 16, nil);
	priv.kq		= strtomp(field[7], nil, 16, nil);
	priv.c2		= strtomp(field[8], nil, 16, nil);

	cert = X509gen(&priv, argv[1], valid, &len);
	if(cert == nil){
		fprint(2, "X509gen failed");
		exits("X509gen");
	}

	if(!PEMflag){
		write(1, cert, len);
	}else{
		pemlen = 2*len;
		buf = malloc(pemlen);
		if(!buf)
			exits("out of memory");
		pemlen = enc64(buf, pemlen, cert, len);
		print("-----BEGIN CERTIFICATE-----\n");
		while(pemlen > 64){
			write(1, buf, 64);
			write(1, "\n", 1);
			buf += 64;
			pemlen -= 64;
		}
		print("%s\n-----END CERTIFICATE-----\n", buf);
	}
	exits("");
}

