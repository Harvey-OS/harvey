typedef struct PmcCtl PmcCtl;
typedef struct PmcCtr PmcCtr;
typedef struct PmcCtlCtrId PmcCtlCtrId;

/*
 * HW performance counters
 */
struct PmcCtl {
	u32int coreno;
	int enab;
	int user;
	int os;
	int nodesc;
	char descstr[KNAMELEN];
};

struct PmcCtr{
	int stale;
	Rendez r;
	u64int ctr;
	int ctrset;
	PmcCtl;
	int ctlset;
};

enum {
	PmcMaxCtrs = 4,
};

struct PmcCore{
	Lock;
	PmcCtr ctr[PmcMaxCtrs];
};

struct PmcCtlCtrId {
	char portdesc[KNAMELEN];
	char archdesc[KNAMELEN];
};

enum {
	PmcIgn = 0,
	PmcGet = 1,
	PmcSet = 2,
};

enum {
	PmcCtlNullval = 0xdead,
};

extern int pmcnregs(void);
extern void pmcinitctl(PmcCtl*);
extern int pmcsetctl(u32int, PmcCtl*, u32int);
extern int pmctrans(PmcCtl*);
extern int pmcgetctl(u32int, PmcCtl*, u32int);
extern int pmcdescstr(char*, int);
extern u64int pmcgetctr(u32int, u32int);
extern int pmcsetctr(u32int, u64int, u32int);

extern void pmcconfigure(void);
