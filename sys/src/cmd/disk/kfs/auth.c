/*
 * Network listening authentication.
 * This and all the other network-related 
 * code is due to Richard Miller.
 */
#include	"all.h"
#include	"9p1.h"

int allownone;
Nvrsafe nvr;
int didread;

/*
 *  create a challenge for a fid space
 */
void
mkchallenge(Chan *cp)
{
	int i;

	if(!didread && readnvram(&nvr, 0) >= 0)
		didread = 1;

	srand(truerand());
	for(i = 0; i < CHALLEN; i++)
		cp->chal[i] = nrand(256);

	cp->idoffset = 0;
	cp->idvec = 0;
}

/*
 *  match a challenge from an attach
 */
Nvrsafe nvr;

int
authorize(Chan *cp, Oldfcall *in, Oldfcall *ou)
{
	Ticket t;
	Authenticator a;
	int x;
	ulong bit;

	if (cp == cons.srvchan)               /* local channel already safe */
		return 1;

	if(wstatallow)		/* set to allow entry during boot */
		return 1;

	if(strcmp(in->uname, "none") == 0)
		return allownone || cp->authed;

	if(in->type == Toattach9p1)
		return 0;

	if(!didread)
		return 0;

	/* decrypt and unpack ticket */
	convM2T(in->ticket, &t, nvr.machkey);
	if(t.num != AuthTs){
print("bad AuthTs num\n");
		return 0;
	}

	/* decrypt and unpack authenticator */
	convM2A(in->auth, &a, t.key);
	if(a.num != AuthAc){
print("bad AuthAc num\n");
		return 0;
	}

	/* challenges must match */
	if(memcmp(a.chal, cp->chal, sizeof(a.chal)) != 0){
print("bad challenge\n");
		return 0;
	}

	/*
	 *  the id must be in a valid range.  the range is specified by a
	 *  lower bount (idoffset) and a bit vector (idvec) where a
	 *  bit set to 1 means unusable
	 */
	lock(&cp->idlock);
	x = a.id - cp->idoffset;
	bit = 1<<x;
	if(x < 0 || x > 31 || (bit&cp->idvec)){
		unlock(&cp->idlock);
		return 0;
	}
	cp->idvec |= bit;

	/* normalize the vector */
	while(cp->idvec&0xffff0001){
		cp->idvec >>= 1;
		cp->idoffset++;
	}
	unlock(&cp->idlock);

	/* ticket name and attach name must match */
	if(memcmp(in->uname, t.cuid, sizeof(in->uname)) != 0){
print("names don't match\n");
		return 0;
	}

	/* copy translated name into input record */
	memmove(in->uname, t.suid, sizeof(in->uname));

	/* craft a reply */
	a.num = AuthAs;
	memmove(a.chal, cp->rchal, CHALLEN);
	convA2M(&a, ou->rauth, t.key);

	cp->authed = 1;
	return 1;
}
