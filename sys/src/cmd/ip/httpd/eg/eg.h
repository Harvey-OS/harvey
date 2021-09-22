typedef struct Index	Index;
struct Index
{
	ulong	start;
	ulong	losac;
	ulong	nhi;
	ulong	hisac;
};

enum
{
	NMaps		= 5,
	NPiece		= 4,
	PieceLog	= 6,
	PieceMask	= (1 << PieceLog) - 1,
	SliceSize	= 1 << (NPiece * PieceLog),

	BlockSize	= 128*1024,

	IndexEnts	= 66 * (SliceSize/BlockSize),
	IndexEntSize	= 8,				/* 2 bytes low sac, 3 each high entries & sac size */
	IndexSize	= IndexEnts * IndexEntSize,
	HeaderId	= 256,
	HeaderSize	= HeaderId + IndexSize,

	ZBase		= 2,			/* base of code to encode 0 runs */
	LitBase		= ZBase-1,		/* base of literal values */
	MaxLit		= 256,

	MaxLeaf		= MaxLit+LitBase,
	MaxHuffBits	= 16,			/* max bits in a huffman code */
	MaxFlatbits	= 8,			/* max bits decoded in flat table */

	CombLog		= 4,
	CombSpace	= 1 << CombLog,		/* mtf speedup indices spacing */
	CombMask	= CombSpace - 1,
};

typedef struct EgMap	EgMap;
struct EgMap
{
	ulong	spread[NPiece][1 << PieceLog];
	int	same[NPiece];
	Index	index[IndexEnts];
};

typedef	struct	Ecor	Ecor;
struct	Ecor
{
	long	k;
	long	p;
	int	n;
};

/*
 * external interface, init and read
 */
typedef	struct	Egdb	Egdb;
struct	Egdb
{
	/* compressed */
	EgMap*	egmap[7];
	int	infd[7];
	int	ncor;
	Ecor*	cor;

	/* uncompressed */
	int	nfd;
	int	fd[10];
};

