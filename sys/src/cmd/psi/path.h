enum elmtype
{
	PA_NONE = -1 ,
	PA_MOVETO ,
	PA_LINETO ,
	PA_CURVETO ,
	PA_CLOSEPATH ,
} ;

struct	element
{
	enum	elmtype	type ;
	struct	pspoint	ppoint ;
	struct	element	*next ;
	struct	element	*prev ;
} ;

struct	path
{
	struct	element	*first ;
	struct	element	*last ;
} ;

void reversepath(struct path *);
void add_element(enum elmtype, struct pspoint);
void rel_path(struct element *);
struct element *flattencurve(struct element *);
struct element *squash(struct element *, struct pspoint[], double, double);
double maxdev(struct pspoint[], double, double);
struct pspoint bezier(struct pspoint[], double);
void arc_line(double, double, double, double);
void arc_part(double ,double ,double ,double ,double);
void join(struct element *, struct element *, struct pspoint);
void outline(struct pspoint, struct pspoint, struct pspoint);
void cap(struct pspoint, struct pspoint, struct pspoint);
void join_round(struct pspoint);
struct pspoint corner(struct pspoint, struct pspoint);
void join_bevel(struct element *, struct element *, struct pspoint);
void join_miter(struct element *, struct element *, struct pspoint);
struct pspoint add_point(struct pspoint, struct pspoint);
struct pspoint sub_point(struct pspoint, struct pspoint);
double hyp_point(struct pspoint, struct pspoint);
struct pspoint makepoint(double, double);
struct pspoint poppoint(char *);
void stroke(void(*)(void));
void stroke_comp(void(*)(void), struct pspoint, int);
struct element *get_elem(void);
void rel_elem(struct element *);
void insert(struct element *, struct element *);
struct path copypath(struct element *);
struct path ncopypath(struct element *);
void appendpath(struct path *, struct path, struct pspoint);
struct pspoint rot90(struct pspoint);
void clean_path(struct path *);
void printpath(char *, struct path, int, int);
void hsbpath(struct path);

extern struct pspoint zeropt;
extern double hypot(double, double);
