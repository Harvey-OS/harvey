typedef struct ROM	ROM;
typedef struct ROMconf	ROMconf;

struct ROM
{
	uint	magic;
	uint	version;
	uint	plugversion;
	uint	monid;
	uint	pad1[3];
	ROMconf	*conf;
	uint	pad2[17];
	void	(*boot)(void*);
	uint	pad3[1];
	void	(*enter)(void);
	int	*msec;
	void	(*exit)(void);
	void	(**callback)(void);
	uint	(*interpret)(void*);
	uint	pad4[2];	
	char	**bootpath;
	char	**bootargs;
	uint	*stdin;
	uint	*stdout;
	uint	(*phandle)(uint);
	uint	(*alloc)(void*, uint);
	void	(*free)(void*);
	uint	(*map)(void*, uint, uint, uint);
	void	(*unmap)(void*, uint);
	uint	(*open)(char*);
	uint	(*close)(uint);
	uint	(*read)(uint, void*, int);
	uint	(*write)(uint, void*, int);
	uint	(*seek)(uint, uint, uint);
	void	(*chain)(void*, uint, void*, void*, uint);
	void	(*release)(void*, uint);
	uint	pad4[15];
	void	(*putcxsegm)(int, ulong, int);
	int	(*startcpu)(uint, uint, uint, uint);
	int	(*stopcpu)(uint);
	int	(*idlecpu)(uint);
	int	(*resumecpu)(uint);
};

struct ROMconf
{
	uint	(*next)(uint);
	uint	(*child)(uint);
	int	(*getproplen)(uint, void*);
	int	(*getprop)(uint, void*, void*);
	int	(*setprop)(uint, void*, void*);
	void*	(*nextprop)(uint, void*);	
};

#define	ROMMAGIC	0x10010407

extern	ROM	*rom;
