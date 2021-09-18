#pragma src "/usr/inferno/libprefab"

typedef struct PElement PElement;
typedef struct PCompound PCompound;
typedef struct Memimage Memimage;

enum
{
	Dirty,
	Clean
};

enum Elementtype	/* same as in prefab.m */
{
	EIcon,
	EText,
	ETitle,
	EHorizontal,
	EVertical,
	ESeparator,
	NEtypes
};

enum Adjust	/* same as in prefab.m */
{
	/* first arg: size of elements */
	Adjpack = 10,	/* leave alone, pack tightly */
	Adjequal = 11,	/* make equal */
	Adjfill = 12,	/* make equal, filling available space */

	/* second arg: position of element within space */
	Adjleft = 20,
	Adjup = 20,
	Adjcenter = 21,
	Adjright = 22,
	Adjdown = 22
};

enum
{
	Maxchars = 128	/* maximum # chars in a word */
};

struct PElement
{
	Prefab_Element	e;
	Point		drawpt;
	Prefab_Compound*highlight;
	List*		first;
	List*		last;
	List*		vfirst;
	List*		vlast;
	int		nkids;
	int		newline;
	int		pkind;	/* for error check against e.kind */
};

struct PCompound
{
	Prefab_Compound	c;
	Display*	display;
};

extern Type*	TLayout;
extern Type*	TElement;
extern Type*	TCompound;

PCompound*	iconbox(Prefab_Environ*, Draw_Point, String*, Draw_Image*, Draw_Image*);
PCompound*	textbox(Prefab_Environ*, Draw_Rect, String*, String*);
PCompound*	layoutbox(Prefab_Environ*, Draw_Rect, String*, List*);
PCompound*	box(Prefab_Environ*, Draw_Point, Prefab_Element*, Prefab_Element*);

PElement*	separatorelement(Prefab_Environ*, Draw_Rect, Draw_Image*, Draw_Image*);
PElement*	iconelement(Prefab_Environ*, Draw_Rect, Draw_Image*, Draw_Image*);
PElement*	textelement(Prefab_Environ*, String*, Draw_Rect, enum Elementtype);
PElement*	layoutelement(Prefab_Environ*, List*, Draw_Rect, enum Elementtype);
PElement*	elistelement(Prefab_Environ*, Prefab_Element*, enum Elementtype);
PElement*	appendelist(Prefab_Element*, Prefab_Element*);

void		drawcompound(Prefab_Compound*);
void		redrawcompound(Image*, Rectangle, Prefab_Compound*);
void		refreshcompound(Image*, Rectangle, void*);
void		drawelement(Prefab_Element*, Image*, Rectangle, int, int);
void		translateelement(Prefab_Element*, Point);
void		adjustelement(Prefab_Element*, int, int);
void		highlightelement(Prefab_Element*, Image*, Prefab_Compound*, int);
void		clipelement(Prefab_Element*, Rectangle);
void		scrollelement(Prefab_Element*, Point, int*);
int		showelement(Prefab_Element*, Prefab_Element*);
void		edge(Prefab_Environ*, Image*, Draw_Rect, Draw_Rect);
int		fitrect(Rectangle*, Rectangle);
Draw_Rect	edgerect(Prefab_Environ*, Draw_Point, Draw_Rect*);
List*		prefabwrap(void*);
List*		listoflayout(Prefab_Style*, String*, int);

extern PElement*	lookupelement(Prefab_Element*);
extern PCompound*	lookupcompound(Prefab_Compound*);
extern PElement*	checkelement(Prefab_Element*);
extern PCompound*	checkcompound(Prefab_Compound*);
extern int		badenviron(Prefab_Environ*, int);
extern PElement*	mkelement(Prefab_Environ*, enum Elementtype);
extern Point		iconsize(Image*);
extern void		localrefreshcompound(Memimage*, Rectangle, void*);
