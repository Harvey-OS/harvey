#pragma	lib	"libip.a"

int	eipconv(void*, Fconv*);
int	parseip(uchar*, char*);
int	parseether(uchar*, char*);
int	myipaddr(uchar*, char*);
int	myetheraddr(uchar*, char*);
void	maskip(uchar*, uchar*, uchar*);
int	equivip(uchar*, uchar*);

extern uchar classmask[4][4];

#define CLASS(p) ((*(uchar*)(p))>>6)
