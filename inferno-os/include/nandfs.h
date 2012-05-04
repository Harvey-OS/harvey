#pragma src "/usr/inferno/libnandfs"

typedef struct Nandfs Nandfs;
typedef struct NandfsTags NandfsTags;

#pragma incomplete Nandfs

enum {
	NandfsL2PageSize = 9,
	NandfsPageSize = 1 << NandfsL2PageSize,
	NandfsAuxiliarySize = 16,
	NandfsFullSize = NandfsPageSize + NandfsAuxiliarySize,
	NandfsPathBits = 26,
	NandfsPathMask = ((1 << NandfsPathBits) - 1),
	NandfsNeraseBits = 18,
	NandfsNeraseMask = ((1 << NandfsNeraseBits) - 1),
};

struct NandfsTags {
	ulong path;	// 26 bits
	ulong nerase;	// 18 bits
	uchar tag;		// 8 bits
	uchar magic;	// 8 bits
};

char *nandfsinit(void*, long, long, char *(*)(void*, void*, long, ulong),
	char *(*)(void*, void*, long, ulong),
	char *(*)(void*, long),
	char *(*)(void*),
	LogfsLowLevel **);
void nandfsfree(Nandfs*);
char *nandfsreadpageauxiliary(Nandfs*, NandfsTags*, long, int, int, LogfsLowLevelReadResult*);
void nandfssetmagic(Nandfs*, void*);
char *nandfswritepageauxiliary(Nandfs*, NandfsTags*, long, int);
char *nandfsreadpage(Nandfs*, void*, NandfsTags*, long, int, int, LogfsLowLevelReadResult*);
char *nandfsreadpagerange(Nandfs*, void*, long, int, int, int, LogfsLowLevelReadResult*);
char *nandfsupdatepage(Nandfs*, void*, ulong, uchar, long, int);

long nandfsgetnerase(Nandfs*, long);
void nandfssetnerase(Nandfs*, long, ulong);
void nandfssetpartial(Nandfs*, long, int);

char *nandfsmarkabsblockbad(Nandfs*, long);

/* low level interface functions */

char *nandfsopen(Nandfs*, long, long, int, int, long*);
short nandfsgettag(Nandfs*, long);
void nandfssettag(Nandfs*, long, short);
long nandfsgetpath(Nandfs*, long);
void nandfssetpath(Nandfs*, long, ulong);
int nandfsgetblockpartialformatstatus(Nandfs*, long);
long nandfsfindfreeblock(Nandfs*, long*);
char *nandfsreadblock(Nandfs*, void*, long, LogfsLowLevelReadResult*);
char *nandfswriteblock(Nandfs*, void*, uchar, ulong, int, long*, long);
char *nandfswritepage(Nandfs*, void*, long, int);
char *nandfseraseblock(Nandfs*, long, void **, int*);
char *nandfsformatblock(Nandfs*, long, uchar, ulong, long, long, int, long*, void*, int*);
char *nandfsreformatblock(Nandfs*, long, uchar, ulong, int, long*, void*, int*);
char *nandfsmarkblockbad(Nandfs*, long);
int nandfsgetpagesize(Nandfs*);
int nandfsgetpagesperblock(Nandfs*);
long nandfsgetblocks(Nandfs*);
long nandfsgetbaseblock(Nandfs*);
int nandfsgetblocksize(Nandfs*);
ulong nandfscalcrawaddress(Nandfs*, long, int);
char *nandfsgetblockstatus(Nandfs*, long, int*, void **, LogfsLowLevelReadResult*);
int nandfscalcformat(Nandfs*, long, long, long, long*, long*, long*);
int nandfsgetopenstatus(Nandfs*);
char *nandfssync(Nandfs*);

/* defined in environment */
void *nandfsrealloc(void*, ulong);
void nandfsfreemem(void*);
