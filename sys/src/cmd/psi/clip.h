struct working {
	struct smoves *sm;
	int used, index, hole, hashole;
	struct working *obj;
	struct element *close;
	struct lstats *sclose;
};
void reorder(void);
int checkint(struct element *);
void
addqhole(struct smoves *);
void
copystr(struct pts *, struct pts *);
int
pcmp(struct pts **, struct pts **);
void
find_inter(struct element *, struct element *, struct lstats *, int,
	 struct element *, struct lstats *, int, int);
void
find_path(struct smoves *, int, int);
struct element *
addqpath(struct element *, struct lstats *, struct element *, int, int);
void
sortq(struct pts [], int);
int
qcmp(struct pts *, struct pts *);
void
collect(struct pts *, double, double,struct lstats *,
	struct lstats *, double , double ,struct element *,struct element *,
	int, int);
struct element *
find_close(struct element *);
struct element *
find_open(struct element *);
int
cadd_element(double, double);
struct element *
rev(struct working *,struct working *, struct path *);
struct ins{
	double value;
	int flag;
};
int icmp(struct ins *, struct ins *);
int
inside(struct pspoint, struct element *, struct lstats *, struct lstats *[],
	int, int, int[]);
int
getslope(struct path *, struct lstats *, struct smoves [],
	int [], struct lstats *[], struct element *[]);

struct working *
copyst(struct working *, struct working *);
void
recomp(struct element *, struct lstats *);
int
getinside(struct element *, struct lstats *[], int,int,
	struct element *, struct lstats *[], int, int, int[],
	double, double, double, double);

void
findlast(struct element *, struct lstats *);
int
equalpath(struct path, struct path);
void
insert_element(struct element *, struct element *);
void
printpath(char *, struct path, int, int);
void
append_elem(struct element *);
void
addpart(struct element *, int);

void
checkit(struct element *, struct working *, struct working *, int);

