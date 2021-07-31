#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <mp.h>
#include <libsec.h>
#include "rsa2any.h"

RSApriv*
getkey(int argc, char **argv, int needprivate, Attr **pa)
{
	char *file, *s, *p;
	int sz;
	RSApriv *key;
	Biobuf *b;
	int regen;
	Attr *a;

	if(argc == 0)
		file = "#d/0";
	else
		file = argv[0];

	key = mallocz(sizeof(RSApriv), 1);
	if(key == nil)
		return nil;

	if((b = Bopen(file, OREAD)) == nil){
		werrstr("open %s: %r", file);
		return nil;
	}
	s = Brdstr(b, '\n', 1);
	if(s == nil){
		werrstr("read %s: %r", file);
		return nil;
	}
	if(strncmp(s, "key ", 4) != 0){
		werrstr("bad key format");
		return nil;
	}

	regen = 0;
	a = _parseattr(s+4);
	if(a == nil){
		werrstr("empty key");
		return nil;
	}
	if((p = _strfindattr(a, "proto")) == nil){
		werrstr("no proto");
		return nil;
	}
	if(strcmp(p, "rsa") != 0){
		werrstr("proto not rsa");
		return nil;
	}
	if((p = _strfindattr(a, "ek")) == nil){
		werrstr("no ek");
		return nil;
	}
	if((key->pub.ek = strtomp(p, &p, 16, nil)) == nil || *p != 0){
		werrstr("bad ek");
		return nil;
	}
	if((p = _strfindattr(a, "n")) == nil){
		werrstr("no n");
		return nil;
	}
	if((key->pub.n = strtomp(p, &p, 16, nil)) == nil || *p != 0){
		werrstr("bad n");
		return nil;
	}
	if((p = _strfindattr(a, "size")) == nil)
		fprint(2, "rsa2any: warning: missing size; will add\n");
	else if((sz = strtol(p, &p, 10)) == 0 || *p != 0)
		fprint(2, "rsa2any: warning: bad size; will correct\n");
	else if(sz != mpsignif(key->pub.n))
		fprint(2, "rsa2any: warning: wrong size (got %d, expected %d); will correct\n",
			sz, mpsignif(key->pub.n));
	if(!needprivate)
		goto call;
	if((p = _strfindattr(a, "!dk")) == nil){
		werrstr("no !dk");
		return nil;
	}
	if((key->dk = strtomp(p, &p, 16, nil)) == nil || *p != 0){
		werrstr("bad !dk");
		return nil;
	}
	if((p = _strfindattr(a, "!p")) == nil){
		werrstr("no !p");
		return nil;
	}
	if((key->p = strtomp(p, &p, 16, nil)) == nil || *p != 0){
		werrstr("bad !p");
		return nil;
	}
	if((p = _strfindattr(a, "!q")) == nil){
		werrstr("no !q");
		return nil;
	}
	if((key->q = strtomp(p, &p, 16, nil)) == nil || *p != 0){
		werrstr("bad !q");
		return nil;
	}
	if((p = _strfindattr(a, "!kp")) == nil){
		fprint(2, "rsa2any: warning: no !kp\n");
		regen = 1;
		goto regen;
	}
	if((key->kp = strtomp(p, &p, 16, nil)) == nil || *p != 0){
		fprint(2, "rsa2any: warning: bad !kp\n");
		regen = 1;	
		goto regen;
	}
	if((p = _strfindattr(a, "!kq")) == nil){
		fprint(2, "rsa2any: warning: no !kq\n");
		regen = 1;	
		goto regen;
	}
	if((key->kq = strtomp(p, &p, 16, nil)) == nil || *p != 0){
		fprint(2, "rsa2any: warning: bad !kq\n");
		regen = 1;	
		goto regen;
	}
	if((p = _strfindattr(a, "!c2")) == nil){
		fprint(2, "rsa2any: warning: no !c2\n");
		regen = 1;	
		goto regen;
	}
	if((key->c2 = strtomp(p, &p, 16, nil)) == nil || *p != 0){
		fprint(2, "rsa2any: warning: bad !c2\n");
		regen = 1;	
		goto regen;
	}
regen:
	if(regen){
		RSApriv *k2;

		k2 = rsafill(key->pub.n, key->pub.ek, key->dk, key->p, key->q);
		if(k2 == nil){
			werrstr("regenerating chinese-remainder parts failed: %r");
			return nil;
		}
		key = k2;
	}
call:
	a = _delattr(a, "ek");
	a = _delattr(a, "n");
	a = _delattr(a, "size");
	a = _delattr(a, "!dk");
	a = _delattr(a, "!p");
	a = _delattr(a, "!q");
	a = _delattr(a, "!c2");
	a = _delattr(a, "!kp");
	a = _delattr(a, "!kq");
	if(pa)
		*pa = a;
	return key;
}

