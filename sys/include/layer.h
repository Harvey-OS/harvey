#pragma	lib	"liblayer.a"

typedef struct Layer Layer;
typedef struct Cover Cover;

typedef enum Lvis
{
	Visible,
	Obscured,
	Invisible,
}Lvis;

struct Layer
{
	Bitmap;			/* Bitmap.cache!=0 ==> layer */
	Layer	*next;		/* next layer from front to back */
	Cover	*cover;		/* layer etc. from which this is derived */
	int	user;		/* a place for the user to stick stuff */
	Lvis	vis;		/* visibility state */
};

struct Cover
{
	Layer	*layer;		/* layer on which these are painted */
	Layer	*front;		/* first sublayer */
	Bitmap	*ground;	/* background texture */
};

extern void	layerop(void (*)(Layer*, Rectangle, Layer*, void*),
			Rectangle, Layer*, void*);
extern Layer	*lalloc(Cover*, Rectangle);
extern void	 lfree(Layer*);
extern void	 ltofront(Layer*);
extern void	 ltoback(Layer*);
extern void	 lupdate(Layer*, Rectangle, Layer *, void *);
extern void	 lcstring(Bitmap*, int, int, uchar*, uchar*, int);

extern void	_bitblt(Bitmap*, Point, Bitmap*, Rectangle, Fcode);
extern void	_linsertback(Layer*);
extern void	_linsertfront(Layer*);
extern void	_ldelete(Layer*);
extern void	_point(Bitmap*, Point, int, Fcode);
extern void	_polysegment(Bitmap*, int, Point*, int, Fcode);
extern Point	_string(Bitmap*, Point, Font*, char*, Fcode);
extern void	_segment(Bitmap*, Point, Point, int, Fcode);
extern void	_texture(Bitmap*, Rectangle, Bitmap*, Fcode);
extern Point	_subfontstring(Bitmap*, Point, Subfont*, char*, Fcode, int);
extern void	_lvis(Layer *);
