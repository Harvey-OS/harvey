#pragma	lib	"libaml.a"
#pragma	src	"/sys/src/libaml"

/*
 *	b	uchar*	buffer		amllen() returns number of bytes
 *	s	char*	string		amllen() is strlen()
 *	n	char*	undefined name	amllen() is strlen()
 *	i	uvlong*	integer
 *	p	void**	package		amllen() is # of elements
 *	r	void*	region
 *	f	void*	field
 *	u	void*	bufferfield
 *	N	void*	name
 *	R	void*	reference
 */
int		amltag(void *);
void*		amlval(void *);
uvlong		amlint(void *);
int		amllen(void *);

void*		amlnew(char tag, int len);

void		amlinit(void);
void		amlexit(void);

int		amlload(uchar *data, int len);
void*		amlwalk(void *dot, char *name);
int		amleval(void *dot, char *fmt, ...);
void		amlenum(void *dot, char *seg, int (*proc)(void *, void *), void *arg);

/*
 * exclude from garbage collection
 */
void		amltake(void *);
void		amldrop(void *);

void*		amlroot;
int		amldebug;

#pragma	varargck	type	"V"	void*
#pragma	varargck	type	"N"	void*

/* to be provided by operating system */
extern void*	amlalloc(int);
extern void	amlfree(void*);

extern void	amldelay(int);	/* microseconds */

enum {
	MemSpace	= 0x00,
	IoSpace		= 0x01,
	PcicfgSpace	= 0x02,
	EbctlSpace	= 0x03,
	SmbusSpace	= 0x04,
	CmosSpace	= 0x05,
	PcibarSpace	= 0x06,
	FixedhwSpace	= 0x08,
	IpmiSpace	= 0x07,
};

typedef struct Amlio Amlio;
struct Amlio
{
	int	space;
	uvlong	off;
	uvlong	len;
	void	*name;
	uchar	*va;

	void	*aux;
	int	(*read)(Amlio *io, void *data, int len, int off);
	int	(*write)(Amlio *io, void *data, int len, int off);
};

extern int	amlmapio(Amlio *io);
extern void	amlunmapio(Amlio *io);
