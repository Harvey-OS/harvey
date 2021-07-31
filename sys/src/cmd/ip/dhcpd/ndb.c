/*
 *  this currently only works for ethernet bootp's -- presotto
 */
#include <u.h>
#include <libc.h>
#include <ip.h>
#include <bio.h>
#include <ndb.h>
#include "dat.h"

Ndb *db;
char *ndbfile;

Iplifc*
findlifc(uchar *ip)
{
	uchar x[IPaddrlen];
	Ipifc *ifc;
	Iplifc *lifc;

	for(ifc = ipifcs; ifc; ifc = ifc->next){
		for(lifc = ifc->lifc; lifc != nil; lifc = lifc->next){
			if(lifc->net[0] == 0)
				continue;
			maskip(ip, lifc->mask, x);
			if(memcmp(x, lifc->net, IPaddrlen) == 0)
				return lifc;
		}
	}
	return nil;
}

int
forme(uchar *ip)
{
	Ipifc *ifc;
	Iplifc *lifc;

	for(ifc = ipifcs; ifc; ifc = ifc->next){
		for(lifc = ifc->lifc; lifc != nil; lifc = lifc->next)
			if(memcmp(ip, lifc->ip, IPaddrlen) == 0)
				return 1;
	}
	return 0;
}

uchar noetheraddr[6];

static void
setipaddr(uchar *addr, char *ip)
{
	if(ipcmp(addr, IPnoaddr) == 0)
		parseip(addr, ip);
}

static void
setipmask(uchar *mask, char *ip)
{
	if(ipcmp(mask, IPnoaddr) == 0)
		parseipmask(mask, ip);
}

/*
 *  do an ipinfo with defaults
 */
int
lookupip(uchar *ipaddr, Info *iip)
{
	char ip[32];
	Ndbtuple *t, *nt;
	char *attrs[32], **p;

	if(db == 0)
		db = ndbopen(ndbfile);
	if(db == 0){
		fprint(2, "can't open db\n");
		return -1;
	}

	p = attrs;
	*p++ = "ip";
	*p++ = "ipmask";
	*p++ = "bootf";
	*p++ = "bootf2";
	*p++ = "rootpath";
	*p++ = "dhcp";
	*p++ = "vendor";
	*p++ = "ether";
	*p++ = "dom";
	*p++ = "@fs";
	*p++ = "@auth";
	*p++ = "@ipgw";
	*p = 0;

	memset(iip, 0, sizeof(*iip));
	snprint(ip, sizeof(ip), "%I", ipaddr);
	t = ndbipinfo(db, "ip", ip, attrs, p - attrs);
	if(t == nil)
		return -1;
	
	for(nt = t; nt != nil; nt = nt->entry){
		if(strcmp(nt->attr, "ip") == 0)
			setipaddr(iip->ipaddr, nt->val);
		else
		if(strcmp(nt->attr, "ipmask") == 0)
			setipmask(iip->ipmask, nt->val);
		else
		if(strcmp(nt->attr, "fs") == 0)
			setipaddr(iip->fsip, nt->val);
		else
		if(strcmp(nt->attr, "auth") == 0)
			setipaddr(iip->auip, nt->val);
		else
		if(strcmp(nt->attr, "ipgw") == 0)
			setipaddr(iip->gwip, nt->val);
		else
		if(strcmp(nt->attr, "ether") == 0){
			if(memcmp(iip->etheraddr, noetheraddr, 6) == 0)
				parseether(iip->etheraddr, nt->val);
			iip->indb = 1;
		}
		else
		if(strcmp(nt->attr, "dhcp") == 0){
			if(iip->dhcpgroup[0] == 0)
				strcpy(iip->dhcpgroup, nt->val);
		}
		else
		if(strcmp(nt->attr, "bootf") == 0){
			if(iip->bootf[0] == 0)
				strcpy(iip->bootf, nt->val);
		}
		else
		if(strcmp(nt->attr, "bootf2") == 0){
			if(iip->bootf2[0] == 0)
				strcpy(iip->bootf2, nt->val);
		}
		else
		if(strcmp(nt->attr, "vendor") == 0){
			if(iip->vendor[0] == 0)
				strcpy(iip->vendor, nt->val);
		}
		else
		if(strcmp(nt->attr, "dom") == 0){
			if(iip->domain[0] == 0)
				strcpy(iip->domain, nt->val);
		}
		else
		if(strcmp(nt->attr, "rootpath") == 0){
			if(iip->rootpath[0] == 0)
				strcpy(iip->rootpath, nt->val);
		}
	}
	ndbfree(t);
	maskip(iip->ipaddr, iip->ipmask, iip->ipnet);
	return 0;
}

static uchar zeroes[6];

/*
 *  lookup info about a client in the database.  Find an address on the
 *  same net as riip.
 */
int
lookup(Bootp *bp, Info *iip, Info *riip)
{
	Ndbtuple *t, *nt;
	Ndbs s;
	char *hwattr;
	char *hwval, hwbuf[33];
	uchar ciaddr[IPaddrlen];

	if(db == 0)
		db = ndbopen(ndbfile);
	if(db == 0){
		fprint(2, "can't open db\n");
		return -1;
	}

	memset(iip, 0, sizeof(*iip));

	/* client knows its address? */
	v4tov6(ciaddr, bp->ciaddr);
	if(validip(ciaddr)){
		if(lookupip(ciaddr, iip) < 0)
			return -1;	/* don't know anything about it */

		if(!samenet(riip->ipaddr, iip)){
			warning(0, "%I not on %I", ciaddr, riip->ipnet);
			return -1;
		}

		/*
		 *  see if this is a masquerade, i.e., if the ether
		 *  address doesn't match what we expected it to be.
		 */
		if(memcmp(iip->etheraddr, zeroes, 6) != 0)
		if(memcmp(bp->chaddr, iip->etheraddr, 6) != 0)
			warning(0, "ciaddr %I rcvd from %E instead of %E",
				ciaddr, bp->chaddr, iip->etheraddr);

		return 0;
	}

	if(bp->hlen > Maxhwlen)
		return -1;
	switch(bp->htype){
	case 1:
		hwattr = "ether";
		hwval = hwbuf;
		snprint(hwbuf, sizeof(hwbuf), "%E", bp->chaddr);
		break;
	default:
		syslog(0, blog, "not ethernet %E, htype %d, hlen %d",
			bp->chaddr, bp->htype, bp->hlen);
		return -1;
	}

	/*
	 *  use hardware address to find an ip address on
	 *  same net as riip
	 */
	t = ndbsearch(db, &s, hwattr, hwval);
	while(t){
		for(nt = t; nt; nt = nt->entry){
			if(strcmp(nt->attr, "ip") != 0)
				continue;
			parseip(ciaddr, nt->val);
			if(lookupip(ciaddr, iip) < 0)
				continue;
			if(samenet(riip->ipaddr, iip)){
				ndbfree(t);
				return 0;
			}
		}
		ndbfree(t);
		t = ndbsnext(&s, hwattr, hwval);
	}
	return -1;
}

/*
 *  return the ip addresses for a type of server for system ip
 */
int
lookupserver(char *attr, uchar **ipaddrs, uchar *addr)
{
	char attrbuf[Ndbvlen+1];
	char *attrs[2];
	char ip[50];
	Ndbtuple *t, *nt;
	int rv = 0;

	if(*attr == '@')
		attrs[0] = attr;
	else {
		snprint(attrbuf, sizeof(attrbuf), "@%s", attr);
		attrs[0] = attrbuf;
	}
	attrs[1] = 0;
	snprint(ip, sizeof(ip), "%I", addr);
	t = ndbipinfo(db, "ip", ip, attrs, 1);
	if(t == nil)
		return rv;
	for(nt = t; rv < 2 && nt != nil; nt = nt->entry)
		if(strcmp(nt->attr, attr) == 0){
			parseip(ipaddrs[rv], nt->val);
			rv++;
		}
	ndbfree(t);
	return rv;
}

/*
 *  just lookup the name
 */
void
lookupname(uchar *ipaddr, char *val)
{
	char ip[32];
	Ndbtuple *t, *nt;
	char *attrs[32], **p;

	p = attrs;
	*p++ = "dom";
	*p = 0;

	snprint(ip, sizeof(ip), "%I", ipaddr);
	t = ndbipinfo(db, "ip", ip, attrs, p - attrs);
	if(t == nil)
		return;
	for(nt = t; nt != nil; nt = nt->entry)
		if(strcmp(nt->attr, "dom") == 0){
			strcpy(val, nt->val);
			break;
		}
	ndbfree(t);
}
