#include <u.h>
#include <libc.h>
#include <auth.h>

enum{
	CHALLEN	= 2*AUTHLEN + 2*NAMELEN,
	RESPLEN	= 2*AUTHLEN + NAMELEN + DESKEYLEN,
	SULEN	= AUTHLEN + NAMELEN + DESKEYLEN,
	MAXLEN	= 2*AUTHLEN + 2*NAMELEN + DESKEYLEN,
};

static void	gethostname(char*, char*);
static int	dofcrypt(int, char*, char*, int);

/*
 * c -> h -> a	KC{KH{RXschal, hchal}, host, RXcchal, cchal}, client
 * a -> h -> c	KC{KH{RXstick, hchal, client, KC}, RXctick, cchal}
 * c -> h	KH{RXstick, hchal, client, KC}
 */
char *
auth(int tohost, char *dialstring)
{
	char *user, buf[MAXLEN], hchal[AUTHLEN], chal[AUTHLEN], host[NAMELEN];
	ulong t;
	int i, j, crypt;

	user = getuser();
	crypt = open("#c/crypt", ORDWR);
	if(crypt < 0)
		return "can't open crypt device";

	gethostname(host, dialstring);
	/*
	 * build the challenge to send to the server
	 */
	if(read(tohost, hchal, AUTHLEN) != AUTHLEN){
		close(crypt);
		return "can't read server challenge";
	}
	i = 0;
	memmove(&buf[i], hchal, AUTHLEN);
	i += AUTHLEN;
	strncpy(&buf[i], host, NAMELEN);
	i += NAMELEN;
	/*
	 * just fake a random challenge
	 */
	buf[i++] = RXcchal;
	t = getpid();
	for(j = 1; j < AUTHLEN/2; j++)
		buf[i++] = t >> (8*j);
	t = time(0);
	for( ; j < AUTHLEN; j++)
		buf[i++] = t >> (8*j);
	memmove(chal, &buf[i-AUTHLEN], AUTHLEN);
	if(dofcrypt(crypt, "E", buf, i) < 0){
		close(crypt);
		return "can't encrypt data";
	}
	strncpy(&buf[i], user, NAMELEN);
	i += NAMELEN;

	/*
	 * send it and get the string to change the user name
	 */
	if(write(tohost, buf, i) != i){
		close(crypt);
		return "can't write challenge";
	}
	if(read(tohost, buf, RESPLEN) != RESPLEN){
		close(crypt);
		return "authentication failed";
	}

	/*
	 * decrypt it and check the challenge string
	 */
	if(dofcrypt(crypt, "D", buf, RESPLEN) < 0){
		close(crypt);
		return "can't decrypt data";
	}
	close(crypt);
	i = AUTHLEN + NAMELEN + DESKEYLEN;
	chal[0] = RXctick;
	if(memcmp(chal, &buf[i], AUTHLEN) != 0)
		return "challenge mismatch";
	if(write(tohost, buf, SULEN) != SULEN)
		return "can't write to server";
	if(read(tohost, buf, 2) != 2 || buf[0] != 'O' || buf[1] != 'K')
		return "server rejected connection";
	return 0;
}

/*
 * extract a host name from the dial string
 * form [net!]host[!service]
 * assumes net! over !service
 * any leading slashes in host are ignored
 * and then if host has a '.', it ends the name
 *
 * does not understand ip addresses like 135.102.117.34
 */
static void
gethostname(char *name, char *dialstring)
{
	char *p, *q;
	int n;

	/*
	 * strip optional net! and then !service
	 */
	n = NAMELEN - 1;
	p = utfrune(dialstring, L'!');
	if(p){
		p++;
		if(q = utfrune(p, L'!'))
			n = q - p;
	}else
		p = dialstring;

	/*
	 * strip leading slashes
	 */
	while((q = utfrune(p, L'/')) && q < p + n){
		q++;
		n -= q - p;
		p = q;
	}
	if(n > NAMELEN - 1)
		n = NAMELEN - 1;
	strncpy(name, p, n);
	name[n] = '\0';
	/*
	 * wipe out \..*
	 */
	if(p = utfrune(name, '.'))
		*p = '\0';
}

/*
 * encrypt/decrypt data using a crypt device
 */
static int
dofcrypt(int fd, char *how, char *s, int n)
{
	
	if(write(fd, how, 1) < 1)
		return -1;
	seek(fd, 0, 0);
	if(write(fd, s, n) < n)
		return -1;
	seek(fd, 0, 0);
	if(read(fd, s, n) < n)
		return -1;
	return n;
}
