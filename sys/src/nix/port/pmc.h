
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
