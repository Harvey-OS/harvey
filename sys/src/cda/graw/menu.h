typedef struct NMitem NMitem;
typedef struct NMenu NMenu;
struct NMitem
{
	char	*text;
	char	*help;
	NMenu	*next;
	void	(*dfn)(NMitem *);
	void	(*bfn)(NMitem *);
	void	(*hfn)(NMitem *);
	long	data;		/* user only */
};

struct NMenu
{
	NMitem	*item;			/* NMitem array, ending with text=0 */
	NMitem	*(*generator)(int,NMitem *);	/* used if item == 0 */
	short	prevhit;		/* private to menuhit() */
	short	prevtop;		/* private to menuhit() */
};

NMitem *mhit(NMenu *, int, int);
screenswap(Bitmap *bp, Rectangle rect, Rectangle screenrect);
