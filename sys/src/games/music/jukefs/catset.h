void catsetrealloc(Catset*, int);
void catsetinit(Catset*, int);
void catsetfree(Catset*t);
void catsetcopy(Catset*, Catset*);
void catsetset(Catset*, int);
int catsetisset(Catset*);
void catsetorset(Catset*, Catset*);
int catseteq(Catset*, Catset*);
