
enum {
	Port4MCP		= 0x80060008,
	Port4SSP		= 0x8007006c,
	AudioDMA		= 0xa,
	SSPXmitDMA		= 0xe,
	SSPRecvDMA		= 0xf,
};

void	dmainit(void);
int		dmaalloc(int, int, int, int, int, ulong, void (*)(void*, ulong), void*);
void	dmareset(int, int, int, int, int, int, ulong);
void	dmafree(int);

ulong	dmastart(int, ulong, int);

void	dmawait(int);
void	dmastop(int);
int		dmaidle(int);
