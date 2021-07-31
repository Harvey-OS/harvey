typedef struct PaqHeader PaqHeader;
typedef struct PaqBlock PaqBlock;
typedef struct PaqTrailer PaqTrailer;
typedef struct PaqDir PaqDir;

enum {
	HeaderMagic = 0x529ab12b,
	HeaderSize = 44,
	BlockMagic = 0x198a1cbf,
	BlockSize = 12,
	TrailerMagic = 0x6b46e688,
	TrailerSize = 28,
	Version = 1,
	MaxBlockSize = 56*1024,
	MinBlockSize = 512,
	MinDirSize = 28,
};

/* block types */
enum {
	DirBlock,
	DataBlock,
	PointerBlock,
};

/* encodings */
enum {
	NoEnc,
	DeflateEnc,
};

struct PaqHeader
{
	ulong	magic;
	ushort	version;
	ushort	blocksize;
	ulong	time;
	char	label[32];
};

struct PaqBlock
{
	ulong	magic;
	ushort	size;		/* data size - always <= blocksize */
	uchar	type;
	uchar	encoding;
	ulong	adler32;	/* applied to unencoded data */
};

struct PaqTrailer
{
	ulong	magic;
	ulong	root;
	uchar	sha1[20];
};

struct PaqDir
{
	ulong	qid;
	ulong	mode;
	ulong	mtime;
	ulong	length;
	ulong	offset;		/* to pointer block */
	char 	*name;
	char	*uid;
	char	*gid;
};
