#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include "whois.h"

Ndbtuple*
addpair(Ndbtuple *old, char *attr, char *val, int newline)
{
	Ndbtuple *t, *nt, *last;

	t = mallocz(sizeof(*t), 1);
	if(t == nil)
		sysfatal("memory");
	strncpy(t->attr, attr, sizeof(t->attr)-1);
	strncpy(t->val, val, sizeof(t->val)-1);
	if(old == nil){
		t->line = t;
		return t;
	}

	last = old;
	for(nt = old; nt->entry != nil; nt = nt->entry)
		if(nt->entry != nt->line)
			last = nt->entry;
	if(newline){
		nt->line = last;
		t->line = t;
	} else {
		nt->line = t;
		t->line = last;
	}
	nt->entry = t;
	return old;
}

int
doquery(char *server, char *format, char *addr, char *buf, int blen, char **lines, int max)
{
	int n, fd, tries;

	fd = -1;
	for(tries = 0; tries < 3; tries++){
		fd = dial(server, 0, 0, 0);
		if(fd >= 0)
			break;
	}
	if(fd < 0){
		return -1;
	}
	if(fprint(fd, format, addr) < 0){
		close(fd);
		return -1;
	}
	n = readn(fd, buf, blen-1);
	close(fd);
	if(n <= 0)
		return -1;
	buf[n] = 0;
	return getfields(buf, lines, max, 0, "\r\n");
}

/* arin example
 *  network:Class-Name:network
 *  network:Auth-Area:0.0.0.0/0
 *  network:ID:NET-UCB-ETHER.0.0.0.0/0
 *  network:Handle:NET-UCB-ETHER
 *  network:Network-Name:UCB-ETHER
 *  network:IP-Network:128.32.0.0/16
 *  network:In-Addr-Server;I:UCB-JADE.0.0.0.0/0
 *  network:In-Addr-Server;I:LILAC.0.0.0.0/0
 *  network:In-Addr-Server;I:VANGOGH.0.0.0.0/0
 *  network:In-Addr-Server;I:UCSF-CGL.0.0.0.0/0
 *  network:IP-Network-Block:128.32.0.0
 *  network:Org-Name:University of California
 *  network:Street-Address:IST-Data Communication and Network Services
 *  network:Street-Address:281 Evans Hall #3806
 *  network:City:Berkeley
 *  network:State:CA
 *  network:Postal-Code:94720-3806
 *  network:Country-Code:US
 *  network:Tech-Contact;I:UCB-NOC-ARIN.0.0.0.0/0
 *  network:Updated:19181231050000000
 *  
 *  network:Class-Name:network
 *  network:Auth-Area:0.0.0.0/0
 *  network:ID:RESERVED-3.0.0.0.0/0
 *  network:Handle:RESERVED-3
 *  network:Network-Name:RESERVED-3
 *  network:IP-Network:128.0.0.0/8
 *  network:IP-Network-Block:128.0.0.0
 *  network:Org-Name:IANA
 *  network:Tech-Contact;I:IANA-ARIN.0.0.0.0/0
 *  network:Updated:19181231050000000
 *  
 *  host:Class-Name:host
 *  host:Auth-Area:0.0.0.0/0
 *  host:ID:VANGOGH.0.0.0.0/0
 *  host:Handle:VANGOGH
 *  host:Host-Name:VANGOGH.CS.BERKELEY.EDU
 *  host:IP-Address:128.32.33.5
 *  host:Updated:19181231050000000
 *  host:Updated-By:jeang@nic.ddn.mil
 *  
 *  %ok
 */

Ndbtuple*
arin(char *ip)
{
	int i, j, n, newline;
	char buf[4096];
	char *l[256];
	char *f[3];
	Ndbtuple *t;

	n = doquery("tcp!whois.arin.net!4321", "%s\n", ip, buf, sizeof(buf), l, nelem(l));
	if(n <= 0)
		return nil;

	/* first look for a referral */
	for(i = 0; i < n; i++){
		if(strstr(l[i], ".ripe.net") || strstr(l[i], ".RIPE.NET"))
			return nil;
		if(strstr(l[i], ".apnic.net") || strstr(l[i], ".APNIC.NET"))
			return nil;
	}

	/* parse the lines */
	t = nil;
	newline = 1;
	for(i = 0; i < n; i++){
		j = getfields(l[i], f, nelem(f), 0, ":");
		if(j < 3){
			newline = 1;
			continue;
		}
		if(cistrcmp(f[0], "network") == 0 && cistrcmp(f[1], "ip-network") == 0){
			t = addpair(t, "iprange", f[2], newline);
			newline = 0;
		}
		if(cistrcmp(f[1], "org-name") == 0){
			t = addpair(t, "org", f[2], newline);
			newline = 0;
		}
		if(cistrcmp(f[1], "city") == 0){
			t = addpair(t, "city", f[2], newline);
			newline = 0;
		}
		if(cistrcmp(f[1], "state") == 0){
			t = addpair(t, "state", f[2], newline);
			newline = 0;
		}
		if(cistrcmp(f[1], "country-code") == 0){
			t = addpair(t, "country", f[2], newline);
			newline = 0;
		}
	}
	return t;
}

/* apnic example
 *  % telnet tcp!whois.apnic.net!whois
 *  connected to tcp!whois.apnic.net!whois on /net/tcp/0
 *  -F 202.12.28.129
 *  
 *  % Rights restricted by copyright. See http://www.apnic.net/db/dbcopyright.html
 *  
 *  *in: 202.12.28.0 - 202.12.28.255
 *  *na: APNIC-AP-JP-TKY
 *  *de: APNIC Pty Ltd - Tokyo Servers
 *  *de: NSPIXP-2, c/- WIDE
 *  *de: KDD Building, Otemachi, Tokyo Central
 *  *cy: JP
 *  *ac: HM20-AP
 *  *tc: NO4-AP
 *  *mb: MAINT-APNIC-AP
 *  *ch: technical@apnic.net 19991207
 *  *so: APNIC
 */
Ndbtuple*
apnic(char *ip)
{
	int i, n;
	char buf[4096];
	char *l[256];
	char *p, *q;
	Ndbtuple *t;

	n = doquery("tcp!whois.apnic.net!whois", "-F %s\n", ip, buf, sizeof(buf), l, nelem(l));
	if(n <= 0)
		return nil;

	/* parse the lines */
	t = nil;
	for(i = 0; i < n; i++){
		p = strchr(l[i], ':');
		if(p == nil)
			continue;
		*p++ = 0;
		while(*p == ' ' || *p == '\t')
			p++;

		if(cistrcmp(l[i], "*in") == 0)
			t = addpair(t, "iprange", p, 0);
		if(cistrcmp(l[i], "*de") == 0)
			t = addpair(t, "org", p, 0);
		if(cistrcmp(l[i], "*cy") == 0)
			t = addpair(t, "country", p, 0);
		if(cistrcmp(l[i], "*ch") == 0){
			q = strchr(p, ' ');
			if(q != nil)
				*q = 0;
			t = addpair(t, "mb", p, 0);
		}
	}

	return t;
}

/* plan9 example
 *  % telnet tcp!olive!whois
 *  202.12.28.129
 *  
 *  *in: 202.12.28.0 - 202.12.28.255
 *  *na: APNIC-AP-JP-TKY
 *  *de: APNIC Pty Ltd - Tokyo Servers
 *  *de: NSPIXP-2, c/- WIDE
 *  *de: KDD Building, Otemachi, Tokyo Central
 *  *cy: JP
 *  *ac: HM20-AP
 *  *tc: NO4-AP
 *  *mb: MAINT-APNIC-AP
 *  *ch: technical@apnic.net 19991207
 */
Ndbtuple*
plan9(char *ip)
{
	int i, n;
	char buf[4096];
	char *l[256];
	char *p, *q;
	Ndbtuple *t;

	n = doquery("tcp!olive!whois", "%s\n", ip, buf, sizeof(buf), l, nelem(l));
	if(n <= 0)
		return nil;

	/* parse the lines */
	t = nil;
	for(i = 0; i < n; i++){
		p = strchr(l[i], ':');
		if(p == nil)
			continue;
		*p++ = 0;
		while(*p == ' ' || *p == '\t')
			p++;

		if(cistrcmp(l[i], "*in") == 0)
			t = addpair(t, "iprange", p, 0);
		if(cistrcmp(l[i], "*de") == 0)
			t = addpair(t, "org", p, 0);
		if(cistrcmp(l[i], "*cy") == 0)
			t = addpair(t, "country", p, 0);
		if(cistrcmp(l[i], "*ch") == 0){
			q = strchr(p, ' ');
			if(q != nil)
				*q = 0;
			t = addpair(t, "mb", p, 0);
		}
	}

	return t;
}

/* ripe example
 *  % telnet tcp!whois.ripe.net!whois
 *  connected to tcp!whois.ripe.net!whois on /net/tcp/0
 *  193.252.19.159
 *  
 *  % Rights restricted by copyright. See http://www.ripe.net/ripencc/pub-services/db/copyright.html
 *  
 *  inetnum:     193.252.19.0 - 193.252.21.255
 *  netname:     FR-WANADOO
 *  descr:       France Telecom Interactive / Wanadoo
 *  country:     FR
 *  admin-c:     SC1509-RIPE
 *  tech-c:      FTI-RIPE
 *  status:      ASSIGNED PA
 *  remarks:     wanadoo.com
 *  mnt-by:      FT-BRX
 *  changed:     noc@rain.fr 19990129
 *  changed:     Patrice.Robert@fti.net 19990219
 *  changed:     noc@rain.fr 19990427
 *  changed:     addr-reg@rain.fr 19990506
 *  source:      RIPE
 *  
 *  route:       193.252.0.0/15
 *  descr:       France Telecom
 *  origin:      AS3215
 *  mnt-by:      RAIN-TRANSPAC
 *  changed:     addr-reg@rain.fr 19990210
 *  source:      RIPE
 *  
 *  role:        Contacts of FTI
 *  address:     France Telecom Interactive
 *  address:     41, rue Camille Desmoulins
 *  address:     92442 Issy Les Moulineaux cedex
 *  phone:       +33 1 41 33 39 00
 *  fax-no:      +33 1 41 33 39 01
 *  e-mail:      postmaster@wanadoo.fr
 *  e-mail:      abuse@wanadoo.fr
 *  trouble:     mail postmaster for ANY problem.
 *  admin-c:     SC1509-RIPE
 *  tech-c:      TEFS1-RIPE
 *  tech-c:      SC1509-RIPE
 *  tech-c:      NS1058-RIPE
 *  tech-c:      CC1215-RIPE
 *  tech-c:      IH678-RIPE
 *  nic-hdl:     FTI-RIPE
 *  notify:      ripe.mnt@fti.net
 *  mnt-by:      FT-INTERACTIVE
 *  changed:     Patrice.Robert@fti.net 19990413
 *  changed:     Patrice.Robert@fti.net 19990415
 *  changed:     Patrice.Robert@fti.net 19990506
 *  changed:     addr-reg@rain.fr 19990921
 *  source:      RIPE
 *  
 *  person:      Sylvain Causse
 *  address:     France Telecom Interactive
 *  address:     41, rue Camille Desmoulins
 *  address:     92442 Issy les Moulineaux cedex
 *  phone:       +33 1 41 33 39 00
 *  fax-no:      +33 1 41 33 26 75
 *  e-mail:      Sylvain.Causse@wanadoo.com
 *  nic-hdl:     SC1509-RIPE
 *  remarks:     Supervision Manager
 *  mnt-by:      FT-INTERACTIVE
 *  changed:     Patrice.Robert@fti.net 19990415
 *  source:      RIPE
 */

Ndbtuple*
ripe(char *ip)
{
	int i, n;
	char buf[4096];
	char *l[256];
	char *p;
	Ndbtuple *t;

	n = doquery("tcp!whois.ripe.net!whois", "%s\n", ip, buf, sizeof(buf), l, nelem(l));
	if(n <= 0)
		return nil;

	/* parse the lines */
	t = nil;
	for(i = 0; i < n; i++){
		p = strchr(l[i], ':');
		if(p == nil)
			continue;
		*p++ = 0;
		while(*p == ' ' || *p == '\t')
			p++;

		if(cistrcmp(l[i], "inetnum") == 0)
			t = addpair(t, "iprange", p, 0);
		if(cistrcmp(l[i], "descr") == 0)
			t = addpair(t, "org", p, 0);
		if(cistrcmp(l[i], "country") == 0)
			t = addpair(t, "country", p, 0);
		if(cistrcmp(l[i], "e-mail") == 0)
			t = addpair(t, "mb", p, 0);
	}

	return t;
}

Ndbtuple*
whois(char *netroot, char *ip)
{
	Ndbtuple *t, **l, *nt, *last;
	int isco;

	// look for a country in the arin database
	t = arin(ip);
	isco = 0;
	l = &t;
	for(; *l != nil; l = &(*l)->entry)
		if(strcmp((*l)->attr, "country") == 0)
			isco = 1;

	// last resort use local whois server which includes the
	// apnic and ripe databases
	if(isco == 0)
		*l = plan9(ip);
	for(; *l != nil; l = &(*l)->entry)
		;

	// do a reverse dns lookup on the address
	*l = dnsquery(netroot, ip, "ptr");

	// now do forward lookup for each domain name
	last = nt = *l;
	for(; *l != nil; l = &(*l)->entry)
		last = *l;
	for(; nt != nil; nt = nt->entry){
		if(strcmp(nt->attr, "dom") == 0){
			*l = dnsquery(netroot, nt->val, "ip");
			for(; *l != nil; l = &(*l)->entry)
				;
		}
		if(nt == last)
			break;
	}

	return t;
}
