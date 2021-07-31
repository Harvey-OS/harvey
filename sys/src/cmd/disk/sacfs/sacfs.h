typedef struct SacHeader SacHeader;
typedef struct SacDir SacDir;

enum {
	Magic = 0x5acf5,
	NAMELEN = 28,
};

struct SacDir
{
	char	name[NAMELEN];
	char	uid[NAMELEN];
	char	gid[NAMELEN];
	uchar	qid[4];
	uchar	mode[4];
	uchar	atime[4];
	uchar	mtime[4];
	uchar	length[8];
	uchar	blocks[8];
};

struct SacHeader
{
	uchar	magic[4];
	uchar	length[8];
	uchar	blocksize[4];
	uchar	md5[16];
};

