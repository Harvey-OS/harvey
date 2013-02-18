// user-level functions, roughly compatible with kernel allocb()

typedef struct Block Block;
struct Block {
	Block	*next;		// next data block
	Block	*flist;		// (used by allocator)
	Block	*list;		// (used by kernel)
	uchar	*rp;		// first unconsumed uchar
	uchar	*wp;		// first empty uchar
	uchar	*lim;		// 1 past the end of the buffer
	uchar	*base;		// start of the buffer
	ulong	pc;		// who allocated this (if ADEBUG)
	ulong	bsz;		// = lim - base
};

// BLEN(b) = number of unconsumed chars in b
#define BLEN(b)	((b)->wp-(b)->rp)

extern Block*	allocb(int n);	// allocate block list with n bytes
extern void	freeb(Block*);	// free block list
extern int	blen(Block*);	// number of unconsumed bytes in block list
extern Block*	concat(Block*);	// merge block list into one block
extern Block*	pullup(Block*, int n); // ensure that n bytes
				// (or all remaining) are in first block
extern Block*	padb(Block*, int n); // put n bytes on front, filled with junk
extern Block*	btrim(Block*, int off, int len); // crop to off...off+len-1
extern Block*	copyb(Block*, int n); // copy n bytes into new block list,
				// padding with trailing zeros if necessary
extern int	pullb(Block**, int n); // discard (up to) n bytes from front
