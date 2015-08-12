/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char copyright[] = "@(#) Copyright (c) 1992, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)mount_nfs.c	8.11 (Berkeley) 5/4/95";
#endif
static const char rcsid[] =
    "$Id: mount_nfs.c,v 1.29 1998/07/06 07:15:53 charnier Exp $";
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syslog.h>

#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_prot.h>

#ifdef ISO
#include <netiso/iso.h>
#endif

#ifdef NFSKERB
#include <kerberosIV/des.h>
#include <kerberosIV/krb.h>
#endif

#include <sys/vnode.h>
#include "9p.h"
#include "9auth.h"
#include "9fs.h"
#include <pwd.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nqnfs.h>

#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sysexits.h>
#include <unistd.h>

#include "mntopts.h"

#define ALTF_BG 0x1
#define ALTF_NOCONN 0x2
#define ALTF_DUMBTIMR 0x4
#define ALTF_INTR 0x8
#define ALTF_KERB 0x10
#define ALTF_NFSV3 0x20
#define ALTF_RDIRPLUS 0x40
#define ALTF_MNTUDP 0x80
#define ALTF_RESVPORT 0x100
#define ALTF_SEQPACKET 0x200
#define ALTF_NQNFS 0x400
#define ALTF_SOFT 0x800
#define ALTF_TCP 0x1000
#define ALTF_PORT 0x2000
#define ALTF_NFSV2 0x4000
#define ALTF_ACREGMIN 0x8000
#define ALTF_ACREGMAX 0x10000
#define ALTF_ACDIRMIN 0x20000
#define ALTF_ACDIRMAX 0x40000

struct mntopt mopts[] = {MOPT_STDOPTS,
                         MOPT_FORCE,
                         MOPT_UPDATE,
                         MOPT_ASYNC,
                         {"bg", 0, ALTF_BG, 1},
                         {"conn", 1, ALTF_NOCONN, 1},
                         {"dumbtimer", 0, ALTF_DUMBTIMR, 1},
                         {"intr", 0, ALTF_INTR, 1},
#ifdef NFSKERB
                         {"kerb", 0, ALTF_KERB, 1},
#endif
                         {"nfsv3", 0, ALTF_NFSV3, 1},
                         {"rdirplus", 0, ALTF_RDIRPLUS, 1},
                         {"mntudp", 0, ALTF_MNTUDP, 1},
                         {"resvport", 0, ALTF_RESVPORT, 1},
#ifdef ISO
                         {"seqpacket", 0, ALTF_SEQPACKET, 1},
#endif
                         {"nqnfs", 0, ALTF_NQNFS, 1},
                         {"soft", 0, ALTF_SOFT, 1},
                         {"tcp", 0, ALTF_TCP, 1},
                         {"port=", 0, ALTF_PORT, 1},
                         {"nfsv2", 0, ALTF_NFSV2, 1},
                         {"acregmin=", 0, ALTF_ACREGMIN, 1},
                         {"acregmax=", 0, ALTF_ACREGMAX, 1},
                         {"acdirmin=", 0, ALTF_ACDIRMIN, 1},
                         {"acdirmax=", 0, ALTF_ACDIRMAX, 1},
                         {NULL}};

struct u9fs_args u9fsdefargs = {
    1, (struct sockaddr*)0, sizeof(struct sockaddr_in), SOCK_SEQPACKET,
    IPPROTO_IL, 0, 0, 0, 0, 0, (char*)0, 0, 0, SOCK_SEQPACKET, IPPROTO_IL,
};

struct nfhret {
	u_long stat;
	int32_t vers;
	int32_t auth;
	int32_t fhsize;
	u_char nfh[NFSX_V3FHMAX];
};
#define DEF_RETRY 10000
#define BGRND 1
#define ISBGRND 2
int retrycnt = DEF_RETRY;
int opflags = 0;
int nfsproto = IPPROTO_UDP;
int mnttcp_ok = 1;
u_short port_no = 0;
enum { ANY, V2, V3 } mountmode = ANY;

#ifdef NFSKERB
char inst[INST_SZ];
char realm[REALM_SZ];
struct {
	u_long kind;
	KTEXT_ST kt;
} ktick;
struct nfsrpc_nickverf kverf;
struct nfsrpc_fullblock kin, kout;
NFSKERBKEY_T kivec;
CREDENTIALS kcr;
struct timeval ktv;
NFSKERBKEYSCHED_T kerb_keysched;
#endif

int getnfsargs __P((char*, struct u9fs_args*));
#ifdef ISO
struct iso_addr* iso_addr __P((const char*));
#endif
void set_rpc_maxgrouplist __P((int));
void usage __P((void)) __dead2;
int xdr_dir __P((XDR*, char*));
int xdr_fh __P((XDR*, struct nfhret*));

void gethostaddr(char* hostp, struct sockaddr_in* saddr);

/*
 * Used to set mount flags with getmntopts.  Call with dir=TRUE to
 * initialize altflags from the current mount flags.  Call with
 * dir=FALSE to update mount flags with the new value of altflags after
 * the call to getmntopts.
 */
static void
setflags(int* altflags, int* nfsflags, int dir)
{
#define F2(af, nf)                                                             \
	if(dir) {                                                              \
		if(*nfsflags & NFSMNT_##nf)                                    \
			*altflags |= ALTF_##af;                                \
		else                                                           \
			*altflags &= ~ALTF_##af;                               \
	} else {                                                               \
		if(*altflags & ALTF_##af)                                      \
			*nfsflags |= NFSMNT_##nf;                              \
		else                                                           \
			*nfsflags &= ~NFSMNT_##nf;                             \
	}
#define F(f) F2(f, f)

	F(NOCONN);
	F(DUMBTIMR);
	F2(INTR, INT);
#ifdef NFSKERB
	F(KERB);
#endif
	F(RDIRPLUS);
	F(RESVPORT);
	F(NQNFS);
	F(SOFT);

#undef F
#undef F2
}

int main(argc, argv) int argc;
char* argv[];
{
	register int c;
	register struct u9fs_args* nfsargsp;
	struct u9fs_args u9fsargs;
	struct nfsd_cargs ncd;
	int mntflags, altflags, i, nfssvc_flag, num;
	char* name, *p, *spec;
	struct vfsconf vfc;
	int error = 0;
	static struct sockaddr_in authaddr;

#ifdef NFSKERB
	uid_t last_ruid;

	last_ruid = -1;
	(void)strcpy(realm, KRB_REALM);
	if(sizeof(struct nfsrpc_nickverf) != RPCX_NICKVERF ||
	   sizeof(struct nfsrpc_fullblock) != RPCX_FULLBLOCK ||
	   ((char*)&ktick.kt) - ((char*)&ktick) != NFSX_UNSIGNED ||
	   ((char*)ktick.kt.dat) - ((char*)&ktick) != 2 * NFSX_UNSIGNED)
		fprintf(stderr, "Yikes! NFSKERB structs not packed!!\n");
#endif /* NFSKERB */
	retrycnt = DEF_RETRY;

	mntflags = 0;
	altflags = 0;
	u9fsargs = u9fsdefargs;
	nfsargsp = &u9fsargs;
	while((c = getopt(argc, argv,
	                  "23a:bcdD:g:I:iKL:lm:No:PpqR:r:sTt:w:x:Uu:")) != -1)
		switch(c) {
		case '2':
			mountmode = V2;
			break;
		case '3':
			mountmode = V3;
			break;
		case 'a':
			num = strtol(optarg, &p, 10);
			if(*p || num < 0)
				errx(1, "illegal -a value -- %s", optarg);
#if 0
			nfsargsp->readahead = num;
			nfsargsp->flags |= NFSMNT_READAHEAD;
#endif
			break;
		case 'b':
			opflags |= BGRND;
			break;
		case 'c':
			nfsargsp->flags |= NFSMNT_NOCONN;
			break;
		case 'D':
			num = strtol(optarg, &p, 10);
			if(*p || num <= 0)
				errx(1, "illegal -D value -- %s", optarg);
#if 0
			nfsargsp->deadthresh = num;
			nfsargsp->flags |= NFSMNT_DEADTHRESH;
#endif
			break;
		case 'd':
			nfsargsp->flags |= NFSMNT_DUMBTIMR;
			break;
		case 'g':
			num = strtol(optarg, &p, 10);
			if(*p || num <= 0)
				errx(1, "illegal -g value -- %s", optarg);
#ifdef __FreeBSD__
			set_rpc_maxgrouplist(num);
#endif
#if 0
			nfsargsp->maxgrouplist = num;
			nfsargsp->flags |= NFSMNT_MAXGRPS;
#endif
			break;
		case 'I':
			num = strtol(optarg, &p, 10);
			if(*p || num <= 0)
				errx(1, "illegal -I value -- %s", optarg);
			nfsargsp->readdirsize = num;
			nfsargsp->flags |= NFSMNT_READDIRSIZE;
			break;
		case 'i':
			nfsargsp->flags |= NFSMNT_INT;
			break;
#ifdef NFSKERB
		case 'K':
			nfsargsp->flags |= NFSMNT_KERB;
			break;
#endif
		case 'L':
			num = strtol(optarg, &p, 10);
			if(*p || num < 2)
				errx(1, "illegal -L value -- %s", optarg);
#if 0
			nfsargsp->leaseterm = num;
			nfsargsp->flags |= NFSMNT_LEASETERM;
#endif
			break;
		case 'l':
			nfsargsp->flags |= NFSMNT_RDIRPLUS;
			break;
#ifdef NFSKERB
		case 'm':
			(void)strncpy(realm, optarg, REALM_SZ - 1);
			realm[REALM_SZ - 1] = '\0';
			break;
#endif
		case 'N':
			nfsargsp->flags &= ~NFSMNT_RESVPORT;
			break;
		case 'o':
			altflags = 0;
			setflags(&altflags, &nfsargsp->flags, TRUE);
			if(mountmode == V2)
				altflags |= ALTF_NFSV2;
			else if(mountmode == V3)
				altflags |= ALTF_NFSV3;
			getmntopts(optarg, mopts, &mntflags, &altflags);
			setflags(&altflags, &nfsargsp->flags, FALSE);
			/*
			 * Handle altflags which don't map directly to
			 * mount flags.
			 */
			if(altflags & ALTF_BG)
				opflags |= BGRND;
			if(altflags & ALTF_MNTUDP)
				mnttcp_ok = 0;
#ifdef ISO
			if(altflags & ALTF_SEQPACKET)
				nfsargsp->sotype = SOCK_SEQPACKET;
#endif
			if(altflags & ALTF_TCP) {
				nfsargsp->sotype = SOCK_STREAM;
				nfsproto = IPPROTO_TCP;
			}
			if(altflags & ALTF_PORT)
				port_no = atoi(strstr(optarg, "port=") + 5);
			mountmode = ANY;
			if(altflags & ALTF_NFSV2)
				mountmode = V2;
			if(altflags & ALTF_NFSV3)
				mountmode = V3;
#if 0
			if(altflags & ALTF_ACREGMIN)
				nfsargsp->acregmin = atoi(strstr(optarg,
				    "acregmin=") + 9);
			if(altflags & ALTF_ACREGMAX)
				nfsargsp->acregmax = atoi(strstr(optarg,
				    "acregmax=") + 9);
			if(altflags & ALTF_ACDIRMIN)
				nfsargsp->acdirmin = atoi(strstr(optarg,
				    "acdirmin=") + 9);
			if(altflags & ALTF_ACDIRMAX)
				nfsargsp->acdirmax = atoi(strstr(optarg,
				    "acdirmax=") + 9);
#endif
			break;
		case 'P':
			/* obsolete for NFSMNT_RESVPORT, now default */
			break;
#ifdef ISO
		case 'p':
			nfsargsp->sotype = SOCK_SEQPACKET;
			break;
#endif
		case 'q':
			mountmode = V3;
			nfsargsp->flags |= NFSMNT_NQNFS;
			break;
		case 'R':
			num = strtol(optarg, &p, 10);
			if(*p || num <= 0)
				errx(1, "illegal -R value -- %s", optarg);
			retrycnt = num;
			break;
		case 'r':
			num = strtol(optarg, &p, 10);
			if(*p || num <= 0)
				errx(1, "illegal -r value -- %s", optarg);
			nfsargsp->rsize = num;
			nfsargsp->flags |= NFSMNT_RSIZE;
			break;
		case 's':
			nfsargsp->flags |= NFSMNT_SOFT;
			break;
		case 'T':
			nfsargsp->sotype = SOCK_STREAM;
			nfsproto = IPPROTO_TCP;
			break;
		case 't':
			num = strtol(optarg, &p, 10);
			if(*p || num <= 0)
				errx(1, "illegal -t value -- %s", optarg);
#if 0
			nfsargsp->timeo = num;
			nfsargsp->flags |= NFSMNT_TIMEO;
#endif
			break;
		case 'w':
			num = strtol(optarg, &p, 10);
			if(*p || num <= 0)
				errx(1, "illegal -w value -- %s", optarg);
			nfsargsp->wsize = num;
			nfsargsp->flags |= NFSMNT_WSIZE;
			break;
		case 'x':
			num = strtol(optarg, &p, 10);
			if(*p || num <= 0)
				errx(1, "illegal -x value -- %s", optarg);
#if 0
			nfsargsp->retrans = num;
			nfsargsp->flags |= NFSMNT_RETRANS;
#endif
			break;
		case 'U':
			mnttcp_ok = 0;
			break;
		case 'u':
			if((p = index(optarg, '@'))) {
				*p++ = 0;
				strncpy(nfsargsp->uname, optarg, U9FS_NAMELEN);
				gethostaddr(p, &authaddr);
				authaddr.sin_family = AF_INET;
				authaddr.sin_port = htons(U9AUTH_ILPORT);
				nfsargsp->authaddr =
				    (struct sockaddr*)&authaddr;
				nfsargsp->authaddrlen = sizeof(authaddr);
			} else
				strncpy(nfsargsp->uname, optarg, U9FS_NAMELEN);
			break;
		default:
			usage();
			break;
		}
	argc -= optind;
	argv += optind;

	if(argc != 2) {
		usage();
		/* NOTREACHED */
	}

	spec = *argv++;
	name = *argv;

	if(!getnfsargs(spec, nfsargsp))
		exit(1);

#ifdef __FreeBSD__
	error = getvfsbyname("u9fs", &vfc);
	if(error && vfsisloadable("nfs")) {
		if(vfsload("nfs"))
			err(EX_OSERR, "vfsload(nfs)");
		endvfsent(); /* clear cache */
		error = getvfsbyname("nfs", &vfc);
	}
	if(error)
		errx(EX_OSERR, "nfs filesystem is not available");

	if(mount(vfc.vfc_name, name, mntflags, nfsargsp))
		err(1, "%s", name);
#else
	if(mount("nfs", name, mntflags, nfsargsp))
		err(1, "%s", name);
#endif
	if(nfsargsp->flags & (NFSMNT_NQNFS | NFSMNT_KERB)) {
		if((opflags & ISBGRND) == 0) {
			if((i = fork())) {
				if(i == -1)
					err(1, "nqnfs 1");
				exit(0);
			}
			(void)setsid();
			(void)close(STDIN_FILENO);
			(void)close(STDOUT_FILENO);
			(void)close(STDERR_FILENO);
			(void)chdir("/");
		}
		openlog("mount_nfs:", LOG_PID, LOG_DAEMON);
		nfssvc_flag = NFSSVC_MNTD;
		ncd.ncd_dirp = name;
		while(nfssvc(nfssvc_flag, (caddr_t)&ncd) < 0) {
			if(errno != ENEEDAUTH) {
				syslog(LOG_ERR, "nfssvc err %m");
				continue;
			}
			nfssvc_flag =
			    NFSSVC_MNTD | NFSSVC_GOTAUTH | NFSSVC_AUTHINFAIL;
#ifdef NFSKERB
			/*
			 * Set up as ncd_authuid for the kerberos call.
			 * Must set ruid to ncd_authuid and reset the
			 * ticket name iff ncd_authuid is not the same
			 * as last time, so that the right ticket file
			 * is found.
			 * Get the Kerberos credential structure so that
			 * we have the session key and get a ticket for
			 * this uid.
			 * For more info see the IETF Draft "Authentication
			 * in ONC RPC".
			 */
			if(ncd.ncd_authuid != last_ruid) {
				char buf[512];
				(void)sprintf(buf, "%s%d", TKT_ROOT,
				              ncd.ncd_authuid);
				krb_set_tkt_string(buf);
				last_ruid = ncd.ncd_authuid;
			}
			setreuid(ncd.ncd_authuid, 0);
			kret = krb_get_cred(NFS_KERBSRV, inst, realm, &kcr);
			if(kret == RET_NOTKT) {
				kret = get_ad_tkt(NFS_KERBSRV, inst, realm,
				                  DEFAULT_TKT_LIFE);
				if(kret == KSUCCESS)
					kret = krb_get_cred(NFS_KERBSRV, inst,
					                    realm, &kcr);
			}
			if(kret == KSUCCESS)
				kret = krb_mk_req(&ktick.kt, NFS_KERBSRV, inst,
				                  realm, 0);

			/*
			 * Fill in the AKN_FULLNAME authenticator and verifier.
			 * Along with the Kerberos ticket, we need to build
			 * the timestamp verifier and encrypt it in CBC mode.
			 */
			if(kret == KSUCCESS &&
			   ktick.kt.length <=
			       (RPCAUTH_MAXSIZ - 3 * NFSX_UNSIGNED) &&
			   gettimeofday(&ktv, (struct timezone*)0) == 0) {
				ncd.ncd_authtype = RPCAUTH_KERB4;
				ncd.ncd_authstr = (u_char*)&ktick;
				ncd.ncd_authlen = nfsm_rndup(ktick.kt.length) +
				                  3 * NFSX_UNSIGNED;
				ncd.ncd_verfstr = (u_char*)&kverf;
				ncd.ncd_verflen = sizeof(kverf);
				memmove(ncd.ncd_key, kcr.session,
				        sizeof(kcr.session));
				kin.t1 = htonl(ktv.tv_sec);
				kin.t2 = htonl(ktv.tv_usec);
				kin.w1 = htonl(NFS_KERBTTL);
				kin.w2 = htonl(NFS_KERBTTL - 1);
				bzero((caddr_t)kivec, sizeof(kivec));

				/*
				 * Encrypt kin in CBC mode using the session
				 * key in kcr.
				 */
				XXX

				    /*
				     * Finally, fill the timestamp verifier into
				     * the
				     * authenticator and verifier.
				     */
				    ktick.kind = htonl(RPCAKN_FULLNAME);
				kverf.kind = htonl(RPCAKN_FULLNAME);
				NFS_KERBW1(ktick.kt) = kout.w1;
				ktick.kt.length = htonl(ktick.kt.length);
				kverf.verf.t1 = kout.t1;
				kverf.verf.t2 = kout.t2;
				kverf.verf.w2 = kout.w2;
				nfssvc_flag = NFSSVC_MNTD | NFSSVC_GOTAUTH;
			}
			setreuid(0, 0);
#endif /* NFSKERB */
		}
	}
	exit(0);
}

/*
 * Return RPC_SUCCESS if server responds.
 */
enum clnt_stat pingnfsserver(addr, version, sotype) struct sockaddr_in* addr;
int version;
int sotype;
{
	struct sockaddr_in sin;
	int tport;
	CLIENT* clp;
	int so = RPC_ANYSOCK;
	enum clnt_stat stat;
	struct timeval pertry, try
		;

	sin = *addr;

	if((tport = port_no ? port_no : pmap_getport(&sin, RPCPROG_NFS, version,
	                                             nfsproto)) == 0) {
		return rpc_createerr.cf_stat;
	}

	sin.sin_port = htons(tport);

	pertry.tv_sec = 10;
	pertry.tv_usec = 0;
	if(sotype == SOCK_STREAM)
		clp = clnttcp_create(&sin, RPCPROG_NFS, version, &so, 0, 0);
	else
		clp = clntudp_create(&sin, RPCPROG_NFS, version, pertry, &so);
	if(clp == NULL)
		return rpc_createerr.cf_stat;

	try
		.tv_sec = 10;
	try
		.tv_usec = 0;
	stat =
	    clnt_call(clp, NFSPROC_NULL, xdr_void, NULL, xdr_void, NULL, try);

	clnt_destroy(clp);

	return stat;
}

int
load_9uid(struct u9fs_args* nfsargsp)
{
	FILE* fd;
	char line[80], *cp;
	int nusers, siz, n;
	struct p9user* p9p, *p9alloc;

	if((fd = fopen("/etc/9uid.conf", "r")) == 0)
		errx(1, "fopen");

	siz = 128;
	if((p9alloc = malloc(siz * sizeof(struct p9user))) == 0)
		errx(1, "malloc");

	nusers = 0;
	p9p = p9alloc;
	while(1) {
		if(nusers < siz) {
			if(fgets(line, 80, fd) == 0)
				break;
			cp = line;
			if(strsep(&cp, " \t") == 0)
				errx(1, "bad format in 9uid.conf");
			p9p->p9_uid = atoi(cp);
			strncpy(p9p->p9_name, line, U9FS_NAMELEN);
			nusers++;
			p9p++;
		} else {
			if((p9p = realloc(
			        p9alloc, 2 * siz * sizeof(struct p9user))) == 0)
				errx(1, "realloc");
			p9alloc = p9p;
			p9p = p9alloc + siz;
			siz <<= 1;
		}
	}

	nfsargsp->nusers = nusers;
	nfsargsp->users = p9alloc;

	return 0;
}

int
passtokey(char* key, char* p)
{
	u_char buf[U9FS_NAMELEN], *t;
	int i, n;

	n = strlen(p);
	if(n >= U9FS_NAMELEN)
		n = U9FS_NAMELEN - 1;
	memset(buf, ' ', 8);
	t = buf;
	strncpy((char*)t, p, n);
	t[n] = '\0';
	memset(key, 0, U9AUTH_DESKEYLEN);
	for(;;) {
		for(i = 0; i < U9AUTH_DESKEYLEN; i++)
			key[i] = (t[i] >> i) + (t[i + 1] << (8 - (i + 1)));
		if(n <= 8)
			return 1;
		n -= 8;
		t += 8;
		if(n < 8) {
			t -= 8 - n;
			n = 8;
		}
		encrypt9(key, t, 8);
	}
	return 1; /* not reached */
}

void
load_9key(struct u9fs_args* nfsargsp)
{
	char* p;

	p = getpass("Plan 9 Password: ");
	passtokey(nfsargsp->key, p);
}

void
gethostaddr(char* hostp, struct sockaddr_in* saddr)
{
	struct hostent* hp;
	/*
	 * Handle an internet host address and reverse resolve it if
	 * doing Kerberos.
	 */
	if(isdigit(*hostp)) {
		if((saddr->sin_addr.s_addr = inet_addr(hostp)) == -1) {
			warnx("bad net address %s", hostp);
		}
	} else if((hp = gethostbyname(hostp)) != NULL)
		memmove(&saddr->sin_addr, hp->h_addr,
		        MIN(hp->h_length, sizeof(saddr->sin_addr)));
	else {
		warnx("can't get net id for host");
	}
}

int getnfsargs(spec, nfsargsp) char* spec;
struct u9fs_args* nfsargsp;
{
	register CLIENT* clp;
	struct hostent* hp;
	static struct sockaddr_in saddr;
#ifdef ISO
	static struct sockaddr_iso isoaddr;
	struct iso_addr* isop;
	int isoflag = 0;
#endif
	struct timeval pertry, try
		;
	enum clnt_stat clnt_stat;
	int so = RPC_ANYSOCK, i, nfsvers, mntvers, orgcnt;
	char* hostp, *delimp;
#ifdef NFSKERB
	char* cp;
#endif
	u_short tport;
	static struct nfhret nfhret;
	static char nam[MNAMELEN + 1];

	strncpy(nam, spec, MNAMELEN);
	nam[MNAMELEN] = '\0';
	if((delimp = strchr(spec, '@')) != NULL) {
		hostp = delimp + 1;
	} else if((delimp = strchr(spec, ':')) != NULL) {
		hostp = spec;
		spec = delimp + 1;
	} else {
		warnx("no <host>:<dirpath> or <dirpath>@<host> spec");
		return (0);
	}
	*delimp = '\0';
/*
 * DUMB!! Until the mount protocol works on iso transport, we must
 * supply both an iso and an inet address for the host.
 */
#ifdef ISO
	if(!strncmp(hostp, "iso=", 4)) {
		u_short isoport;

		hostp += 4;
		isoflag++;
		if((delimp = strchr(hostp, '+')) == NULL) {
			warnx("no iso+inet address");
			return (0);
		}
		*delimp = '\0';
		if((isop = iso_addr(hostp)) == NULL) {
			warnx("bad ISO address");
			return (0);
		}
		memset(&isoaddr, 0, sizeof(isoaddr));
		memmove(&isoaddr.siso_addr, isop, sizeof(struct iso_addr));
		isoaddr.siso_len = sizeof(isoaddr);
		isoaddr.siso_family = AF_ISO;
		isoaddr.siso_tlen = 2;
		isoport = htons(NFS_PORT);
		memmove(TSEL(&isoaddr), &isoport, isoaddr.siso_tlen);
		hostp = delimp + 1;
	}
#endif /* ISO */

	gethostaddr(hostp, &saddr);
#ifdef NFSKERB
	if((nfsargsp->flags & NFSMNT_KERB)) {
		if((hp = gethostbyaddr((char*)&saddr.sin_addr.s_addr,
		                       sizeof(u_long), AF_INET)) ==
		   (struct hostent*)0) {
			warnx("can't reverse resolve net address");
			return (0);
		}
		memmove(&saddr.sin_addr, hp->h_addr,
		        MIN(hp->h_length, sizeof(saddr.sin_addr)));
		strncpy(inst, hp->h_name, INST_SZ);
		inst[INST_SZ - 1] = '\0';
		if(cp = strchr(inst, '.'))
			*cp = '\0';
	}
#endif /* NFSKERB */

	orgcnt = retrycnt;
tryagain:
	if(mountmode == ANY || mountmode == V3) {
		nfsvers = 3;
		mntvers = 3;
		nfsargsp->flags |= NFSMNT_NFSV3;
	} else {
		nfsvers = 2;
		mntvers = 1;
		nfsargsp->flags &= ~NFSMNT_NFSV3;
	}
	tport = port_no ? port_no : U9FS_PORT;

#if 0
	nfhret.stat = EACCES;	/* Mark not yet successful */
	while (retrycnt > 0) {
		saddr.sin_family = AF_INET;
		saddr.sin_port = htons(PMAPPORT);
		if ((tport = port_no ? port_no :
		     pmap_getport(&saddr, RPCPROG_NFS,
		    		  nfsvers, nfsproto)) == 0) {
			if ((opflags & ISBGRND) == 0)
				clnt_pcreateerror("NFS Portmap");
		} else {
			/*
			 * First ping the nfs server to see if it supports
			 * the version of the protocol we want to use.
			 */
			clnt_stat = pingnfsserver(&saddr, nfsvers,
						  nfsargsp->sotype);
			if (clnt_stat == RPC_PROGVERSMISMATCH) {
				if (mountmode == ANY) {
					mountmode = V2;
					goto tryagain;
				} else {
					errx(1, "can't contact NFS server");
				}
			}
			saddr.sin_port = 0;
			pertry.tv_sec = 10;
			pertry.tv_usec = 0;
			if (mnttcp_ok && nfsargsp->sotype == SOCK_STREAM)
			    clp = clnttcp_create(&saddr, RPCPROG_MNT, mntvers,
				&so, 0, 0);
			else
			    clp = clntudp_create(&saddr, RPCPROG_MNT, mntvers,
				pertry, &so);
			if (clp == NULL) {
				if ((opflags & ISBGRND) == 0)
					clnt_pcreateerror("Cannot MNT RPC");
			} else {
				clp->cl_auth = authunix_create_default();
				try.tv_sec = 10;
				try.tv_usec = 0;
				if (nfsargsp->flags & NFSMNT_KERB)
				    nfhret.auth = RPCAUTH_KERB4;
				else
				    nfhret.auth = RPCAUTH_UNIX;
				nfhret.vers = mntvers;
				clnt_stat = clnt_call(clp, RPCMNT_MOUNT,
				    xdr_dir, spec, xdr_fh, &nfhret, try);
				if (clnt_stat != RPC_SUCCESS) {
					if (clnt_stat == RPC_PROGVERSMISMATCH) {
						if (mountmode == ANY) {
							mountmode = V2;
							goto tryagain;
						} else {
							errx(1, "%s",
							     clnt_sperror(clp, "MNT RPC"));
						}
					}
					if ((opflags & ISBGRND) == 0)
						warnx("%s", clnt_sperror(clp,
						    "bad MNT RPC"));
				} else {
					auth_destroy(clp->cl_auth);
					clnt_destroy(clp);
					retrycnt = 0;
				}
			}
		}
		if (--retrycnt > 0) {
			if (opflags & BGRND) {
				opflags &= ~BGRND;
				if ((i = fork())) {
					if (i == -1)
						err(1, "nqnfs 2");
					exit(0);
				}
				(void) setsid();
				(void) close(STDIN_FILENO);
				(void) close(STDOUT_FILENO);
				(void) close(STDERR_FILENO);
				(void) chdir("/");
				opflags |= ISBGRND;
			}
			sleep(60);
		}
	}
	if (nfhret.stat) {
		if (opflags & ISBGRND)
			exit(1);
		warnx("can't access %s: %s", spec, strerror(nfhret.stat));
		return (0);
	}
#endif
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(tport);
#ifdef ISO
	if(isoflag) {
		nfsargsp->addr = (struct sockaddr*)&isoaddr;
		nfsargsp->addrlen = sizeof(isoaddr);
	} else
#endif /* ISO */
	    {
		nfsargsp->addr = (struct sockaddr*)&saddr;
		nfsargsp->addrlen = sizeof(saddr);
	}
#if 0
	nfsargsp->fh = nfhret.nfh;
#endif
	nfsargsp->fhsize = nfhret.fhsize;
	nfsargsp->hostname = nam;

	load_9key(nfsargsp);
	if(load_9uid(nfsargsp))
		errx(1, "can't load 9uid.conf");

	return (1);
}

/*
 * xdr routines for mount rpc's
 */
int xdr_dir(xdrsp, dirp) XDR* xdrsp;
char* dirp;
{
	return (xdr_string(xdrsp, &dirp, RPCMNT_PATHLEN));
}

int xdr_fh(xdrsp, np) XDR* xdrsp;
register struct nfhret* np;
{
	register int i;
	int32_t auth, authcnt, authfnd = 0;

	if(!xdr_u_long(xdrsp, &np->stat))
		return (0);
	if(np->stat)
		return (1);
	switch(np->vers) {
	case 1:
		np->fhsize = NFSX_V2FH;
		return (xdr_opaque(xdrsp, (caddr_t)np->nfh, NFSX_V2FH));
	case 3:
		if(!xdr_long(xdrsp, &np->fhsize))
			return (0);
		if(np->fhsize <= 0 || np->fhsize > NFSX_V3FHMAX)
			return (0);
		if(!xdr_opaque(xdrsp, (caddr_t)np->nfh, np->fhsize))
			return (0);
		if(!xdr_long(xdrsp, &authcnt))
			return (0);
		for(i = 0; i < authcnt; i++) {
			if(!xdr_long(xdrsp, &auth))
				return (0);
			if(auth == np->auth)
				authfnd++;
		}
		/*
		 * Some servers, such as DEC's OSF/1 return a nil authenticator
		 * list to indicate RPCAUTH_UNIX.
		 */
		if(!authfnd && (authcnt > 0 || np->auth != RPCAUTH_UNIX))
			np->stat = EAUTH;
		return (1);
	};
	return (0);
}

void
usage()
{
	(void)fprintf(
	    stderr, "%s\n%s\n%s\n%s\n", "usage: mount_nfs [-23KNPTUbcdilqs] "
	                                "[-D deadthresh] [-I readdirsize]",
	    "                 [-L leaseterm] [-R retrycnt] [-a maxreadahead]",
	    "                 [-g maxgroups] [-m realm] [-o options] [-r "
	    "readsize]",
	    "                 [-t timeout] [-w writesize] [-x retrans] "
	    "rhost:path node");
	exit(1);
}
