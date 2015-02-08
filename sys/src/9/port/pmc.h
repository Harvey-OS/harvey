/* 
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */


enum{
	PmcCtlNullval = 0xdead,
};

typedef struct PmcCtlCtrId PmcCtlCtrId;


struct PmcCtlCtrId {
	char portdesc[KNAMELEN];
	char archdesc[KNAMELEN];
};

int		pmcnregs(void);
int		pmcsetctl(u32int coreno, PmcCtl *p, u32int regno);
int		pmctrans(PmcCtl *p);
int		pmcgetctl(u32int coreno, PmcCtl *p, u32int regno);
int		pmcdescstr(char *str, int nstr);
int		pmcctlstr(char *str, int nstr, PmcCtl *p);
u64int	pmcgetctr(u32int coreno, u32int regno);
int		pmcsetctr(u32int coreno, u64int v, u32int regno);

void		pmcupdate(Mach *m);
extern	void (*_pmcupdate)(Mach *m);
