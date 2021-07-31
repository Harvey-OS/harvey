typedef struct NMitem
{
	char	*text;
	char	*help;
	struct NMenu *next;
	void	(*dfn)(struct NMitem *);
	void	(*bfn)(struct NMitem *);
	void	(*hfn)(struct NMitem *);
	long	data;		/* user only */
} NMitem;

typedef struct NMenu
{
	NMitem	*item;			/* NMitem array, ending with text=0 */
	NMitem	*(*generator)(int, NMitem *);	/* used if item == 0 */
	short	prevhit;		/* private to menuhit() */
	short	prevtop;		/* private to menuhit() */
} NMenu;

NMitem *mhit(NMenu *, int, int, Mouse *);
