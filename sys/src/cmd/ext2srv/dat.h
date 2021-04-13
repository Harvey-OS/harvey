/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Xfs	Xfs;
typedef struct Xfile	Xfile;
typedef struct Iobuf	Iobuf;
typedef struct Ext2 Ext2;

typedef struct SuperBlock SuperBlock;
typedef struct GroupDesc GroupDesc;
typedef struct Inode Inode;
typedef struct DirEntry DirEntry;

#define SECTORSIZE	512
#define OFFSET_SUPER_BLOCK	1024

#define EXT2_SUPER_MAGIC	0xEF53
#define EXT2_MIN_BLOCK_SIZE  1024
#define EXT2_MAX_BLOCK_SIZE  4096
#define EXT2_ROOT_INODE	2
#define EXT2_FIRST_INO		11
#define EXT2_VALID_FS	0x0001
#define EXT2_ERROR_FS	0x0002

/*
 * Structure of the super block
 */
struct SuperBlock {
	u32	s_inodes_count;		/* Inodes count */
	u32	s_blocks_count;		/* Blocks count */
	u32	s_r_blocks_count;	/* Reserved blocks count */
	u32	s_free_blocks_count;	/* Free blocks count */
	u32	s_free_inodes_count;	/* Free inodes count */
	u32	s_first_data_block;	/* First Data Block */
	u32	s_log_block_size;	/* Block size */
	int	s_log_frag_size;	/* Fragment size */
	u32	s_blocks_per_group;	/* # Blocks per group */
	u32	s_frags_per_group;	/* # Fragments per group */
	u32	s_inodes_per_group;	/* # Inodes per group */
	u32	s_mtime;		/* Mount time */
	u32	s_wtime;		/* Write time */
	u16	s_mnt_count;		/* Mount count */
	short	s_max_mnt_count;	/* Maximal mount count */
	u16	s_magic;		/* Magic signature */
	u16	s_state;		/* File system state */
	u16	s_errors;		/* Behaviour when detecting errors */
	u16	s_pad;
	u32	s_lastcheck;		/* time of last check */
	u32	s_checkinterval;	/* max. time between checks */
	u32	s_creator_os;		/* OS */
	u32	s_rev_level;		/* Revision level */
	u16	s_def_resuid;		/* Default uid for reserved blocks */
	u16	s_def_resgid;		/* Default gid for reserved blocks */
	u32	s_reserved[235];	/* Padding to the end of the block */
};

/*
 * Structure of a blocks group descriptor
 */
struct GroupDesc
{
	u32	bg_block_bitmap;		/* Blocks bitmap block */
	u32	bg_inode_bitmap;		/* Inodes bitmap block */
	u32	bg_inode_table;		/* Inodes table block */
	u16	bg_free_blocks_count;	/* Free blocks count */
	u16	bg_free_inodes_count;	/* Free inodes count */
	u16	bg_used_dirs_count;	/* Directories count */
	u16	bg_pad;
	u32	bg_reserved[3];
};

/*
 * Constants relative to the data blocks
 */
#define	EXT2_NDIR_BLOCKS		12
#define	EXT2_IND_BLOCK			EXT2_NDIR_BLOCKS
#define	EXT2_DIND_BLOCK			(EXT2_IND_BLOCK + 1)
#define	EXT2_TIND_BLOCK			(EXT2_DIND_BLOCK + 1)
#define	EXT2_N_BLOCKS			(EXT2_TIND_BLOCK + 1)

/*
 * Structure of an inode on the disk
 */
struct Inode {
	u16 i_mode;		/* File mode */
	u16 i_uid;		/* Owner Uid */
	u32  i_size;		/* Size in bytes */
	u32  i_atime;		/* Access time */
	u32 i_ctime;		/* Creation time */
	u32  i_mtime;		/* Modification time */
	u32  i_dtime;		/* Deletion Time */
	u16 i_gid;		/* Group Id */
	u16 i_links_count;	/* Links count */
	u32  i_blocks;	/* Blocks count */
	u32  i_flags;		/* File flags */
	u32 osd1;
	u32	i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
	u32	i_version;	/* File version (for NFS) */
	u32	i_file_acl;	/* File ACL */
	u32	i_dir_acl;	/* Directory ACL */
	u32	i_faddr;		/* Fragment address */
	u8 osd2[12];
};

/*
 * Structure of a directory entry
 */
#define EXT2_NAME_LEN 255
#define DIR_REC_LEN(name_len)	(((name_len) + 8 + 3) & ~3)

struct DirEntry {
	u32	inode;			/* Inode number */
	u16	rec_len;		/* Directory entry length */
	u8	name_len;		/* Name length */
	u8	reserved;
	char	name[EXT2_NAME_LEN];	/* File name */
};

#define S_IFMT  00170000
#define S_IFLNK	 0120000
#define S_IFREG  0100000
#define S_IFDIR  0040000

#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)

#define DEFAULT_UID	200
#define DEFAULT_GID	100

struct Iobuf
{
	Xfs *dev;
	long	addr;
	Iobuf *next;
	Iobuf *prev;
	Iobuf *hash;
	int busy;
	int dirty;
	char *iobuf;
};

struct Xfs{
	Xfs *next;
	char *name;		/* of file containing external f.s. */
	Qid	qid;		/* of file containing external f.s. */
	long	ref;		/* attach count */
	Qid	rootqid;	/* of plan9 constructed root directory */
	short	dev;
	short	fmt;
	void *ptr;

	/* data from super block */

	int block_size;
	int desc_per_block;
	int inodes_per_group;
	int inodes_per_block;
	int addr_per_block;
	int blocks_per_group;

	int ngroups;
	int superaddr, superoff;
	int grpaddr;
};

struct Xfile{
	Xfile *next;		/* in hash bucket */
	long	client;
	long	fid;
	Xfs *	xf;
	void *	ptr;

	u32 inbr;		/* inode nbr */
	u32 pinbr;	/* parrent inode */
	u32 bufaddr;	/* addr of inode block */
	u32 bufoffset;
	int root;		/* true on attach for ref count */
	int dirindex;	/* next dir entry to read */
};

#define EXT2_SUPER		1
#define EXT2_DESC		2
#define EXT2_BBLOCK	3
#define EXT2_BINODE	4
#define EXT2_UNKNOWN 5

struct Ext2{
	char type;
	union{
		SuperBlock *sb;
		GroupDesc *gd;
		char *bmp;
	}u;
	Iobuf *buf;
};

#define DESC_ADDR(xf,n)		( (xf)->grpaddr + ((n)/(xf)->desc_per_block) )
#define DESC_OFFSET(xf,d,n)	( ((GroupDesc *)(d)) + ((n)%(xf)->desc_per_block) )

enum{
	Asis, Clean, Clunk
};

enum{
	Enevermind,
	Eformat,
	Eio,
	Enomem,
	Enonexist,
	Eexist,
	Eperm,
	Enofilsys,
	Eauth,
	Enospace,
	Elink,
	Elongname,
	Eintern,
	Ecorrupt,
	Enotclean
};

extern int	chatty;
extern int	errno;
extern char	*deffile;
extern int rdonly;
